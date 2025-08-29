/**
*  Copyright (C) 2015 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "repo_model_import_3drepo.h"
#include <sstream>
#include <fstream>
#include <stack>
#include <iostream>
#include <istream>
#include <ranges>
#include <iomanip>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "repo/manipulator/modelutility/repo_scene_builder.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/model/bson/repo_node_material.h"
#include "repo/core/model/bson/repo_node_mesh.h"
#include "repo/core/model/bson/repo_node_metadata.h"
#include "repo/core/model/bson/repo_node_transformation.h"
#include "repo/core/model/bson/repo_node_texture.h"
#include "repo/error_codes.h"
#include <repo_log.h>
#include "repoHelper/parser.h"

using namespace repo::core::model;
using namespace repo::manipulator::modelconvertor;
using namespace repo::manipulator::modelconvertor::repoHelper;
using namespace rapidjson;

const static int REPO_VERSION_LENGTH = 6;

RepoModelImport::RepoModelImport(const ModelImportConfig& settings) :
	AbstractModelImport(settings)
{
	scene = nullptr;
}

RepoModelImport::~RepoModelImport()
{
}

struct View
{
	size_t begin;
	size_t end;

	View(const Range& range)
		:begin(range.start),
		end(range.end)
	{
	}

	size_t size()
	{
		return end - begin;
	}

	virtual ~View() = default; // Required for the subclass destructors to be called and so to release the shared pointers.
};

template<typename T>
struct MeshAttributeView : public View
{
	std::shared_ptr<repo::core::model::MeshNode> node;

	MeshAttributeView(const Range& range, std::shared_ptr<repo::core::model::MeshNode> node):
		View(range),
		node(node)
	{}

	virtual std::vector<T> vector(char* data)
	{
		std::vector<T> vector;
		vector.resize(size() / sizeof(T));
		memcpy(vector.data(), data, size());
		return vector;
	}
};

template<typename T>
struct TransformedAttributeView : public MeshAttributeView<T>
{
	repo::lib::RepoMatrix matrix;

	TransformedAttributeView(const Range& range, std::shared_ptr<repo::core::model::MeshNode> node, repo::lib::RepoMatrix matrix):
		MeshAttributeView<T>(range, node),
		matrix(matrix)
	{}

	std::vector<T> vector(char* data) override
	{
		auto vector = MeshAttributeView<T>::vector(data);
		for (auto& v : vector) {
			v = matrix * v;
		}
		return vector;
	}
};

struct VerticesView : public TransformedAttributeView<repo::lib::RepoVector3D64>
{
	using TransformedAttributeView::TransformedAttributeView;
};

struct NormalsView : public TransformedAttributeView<repo::lib::RepoVector3D>
{
	using TransformedAttributeView::TransformedAttributeView;

	std::vector<repo::lib::RepoVector3D> vector(char* data) override
	{
		auto vector = TransformedAttributeView<repo::lib::RepoVector3D>::vector(data);
		for (auto& v : vector) {
			v.normalize();
		}
		return vector;
	}
};

struct UvsView : public MeshAttributeView<repo::lib::RepoVector2D>
{
	using MeshAttributeView::MeshAttributeView;
};

struct TextureView : public View
{
	TextureRecord record;

	TextureView(const TextureRecord& record):
		View(record.bytes),
		record(record)
	{
	}
};

struct IndicesView : public MeshAttributeView<uint32_t>
{
	repo::core::model::MeshNode::Primitive primitive;
	IndicesView(const Range& range, std::shared_ptr<repo::core::model::MeshNode> node, repo::core::model::MeshNode::Primitive primitive)
		:MeshAttributeView(range, node),
		primitive(primitive)
	{
	}
};

/*
* A subclass of RepoSceneBuilder with a dictionary for mapping local indices.
*/
class Builder : private repo::manipulator::modelutility::RepoSceneBuilder, public AbstractBuilder
{
public:

	Builder(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
		const std::string& database,
		const std::string& project,
		const repo::lib::RepoUUID& revisionId) :
		RepoSceneBuilder(handler, database, project, revisionId),
		minBufferSize(0),
		numMaterials(0)
	{
		createIndexes();
	}

	struct Ids
	{
		repo::lib::RepoUUID uniqueId;
		repo::lib::RepoUUID sharedId;

		Ids()
			:uniqueId(repo::lib::RepoUUID::createUUID()),
			sharedId(repo::lib::RepoUUID::createUUID())
		{
		}
	};

	std::unordered_map<int, repo::lib::RepoUUID> transformSharedIds;
	std::unordered_map<int, repo::lib::RepoMatrix> modelToWorld;
	std::unordered_map<int, Ids> materialIds;
	std::unordered_map<int, Ids> textureIds;
	std::unordered_map<repo::lib::RepoUUID, std::vector<std::shared_ptr<repo::core::model::MeshNode>>, repo::lib::RepoUUIDHasher> matToMeshNodes;	
	size_t numMaterials;
	std::vector<View*> dataMap;
	size_t minBufferSize;
	repo::lib::RepoVector3D64 offset; // Applied to the vertex data

	/* 
	* Used to set the parents once all nodes have been read in. This is used
	* instead of RepoSceneBuilder's addParent until the end, because the Ids may
	* be created before the actual nodes themselves.
	*/
	std::vector<std::pair<repo::lib::RepoUUID, repo::lib::RepoUUID>> references; // From uniqueId to sharedId

	void getParent(int id, std::vector<repo::lib::RepoUUID>& parents, repo::lib::RepoMatrix& matrix)
	{
		auto it = transformSharedIds.find(id);
		if (it != transformSharedIds.end())
		{
			parents.push_back(it->second);
		}
		if (id >= 0) {
			matrix = modelToWorld[id];
		}
	}

	void createNode(const NodeRecord& r) override
	{
		repo::lib::RepoMatrix matrix;
		std::vector<repo::lib::RepoUUID> parentIds;
		getParent(r.parentId, parentIds, matrix);

		bool isEntity = !r.name.empty() || !r.matrix.isIdentity();
		if (isEntity)
		{
			auto node = repo::core::model::RepoBSONFactory::makeTransformationNode(
				{},
				r.name,
				parentIds
			);
			transformSharedIds[r.id] = node.getSharedID();
			parentIds.clear();
			parentIds.push_back(node.getSharedID());
			addNode(node);
		}
		else
		{
			transformSharedIds[r.id] = parentIds[0]; // If the TransformationNode is effectively a noop, any child nodes go directly to its parent
		}

		// For BIM004 and below, the importer bakes everything on import, including the root transform

		if (!r.matrix.isIdentity())
		{
			matrix = matrix * r.matrix;
		}

		modelToWorld[r.id] = matrix;

		// Take care that this comes before the metadata node creation, as it can modify
		// the parentIds vector.

		if (r.geometry.hasGeometry)
		{
			// When the views are deleted, the shared references to the MeshNode will
			// be deleted, and after the last one is gone the node will be free to be
			// committed by RepoSceneBuilder

			auto node = addNode(MeshNode());
			node->setUniqueID(repo::lib::RepoUUID::createUUID());
			node->setSharedID(repo::lib::RepoUUID::createUUID());
			node->setParents(parentIds);
			
			if (!isEntity) {
				parentIds = { node->getSharedID() };
			}

			dataMap.push_back(new VerticesView(r.geometry.vertices, node, matrix));

			if (r.geometry.indices.size()) {
				dataMap.push_back(new IndicesView(r.geometry.indices, node, r.geometry.primitive));
			}

			if (r.geometry.normals.size()) {
				dataMap.push_back(new NormalsView(r.geometry.normals, node, matrix.invert().transpose().rotation()));
			}

			if (r.geometry.uv.size()) {
				dataMap.push_back(new UvsView(r.geometry.uv, node));
			}

			auto& material = materialIds[r.geometry.material];
			references.push_back({ material.uniqueId, node->getSharedID() });		

			auto& nodes = matToMeshNodes[material.uniqueId];
			nodes.push_back(node);			
		}

		if (r.metadata.size())
		{
			auto node = repo::core::model::RepoBSONFactory::makeMetaDataNode(r.metadata, {}, parentIds);
			addNode(node);
		}
	}

	// It is expected that materials are added in the order they are indexed by
	// the nodes
	void createMaterial(repo::lib::repo_material_t material, int texture) override
	{
		auto n = repo::core::model::RepoBSONFactory::makeMaterialNode(material, {}, {});
		auto& ids = materialIds[numMaterials++];
		n.setUniqueID(ids.uniqueId);
		n.setSharedID(ids.sharedId);
		addNode(n);
				
		auto matStruct = n.getMaterialStruct();
		auto& nodes = matToMeshNodes[ids.uniqueId];
		for (auto& node : nodes) {
			node->setMaterial(matStruct);
		}

		if (texture != -1) {
			auto texId = textureIds[texture].uniqueId;
			references.push_back({ texId, ids.sharedId });
			for (auto& node : nodes) {
				node->setTextureId(texId);
			}
		}
		
		// Remove entry from the map to release nodes
		matToMeshNodes.erase(ids.uniqueId);		
	}

	void createTexture(const TextureRecord& t) override
	{
		if (t.isOK()) {
			dataMap.push_back(new TextureView(t)); // The node and blobs are created together for textures
		}
		else {
			setMissingTextures();
		}
	}

	void prepareDataMap()
	{
		std::ranges::sort(dataMap, {}, &View::begin);

		minBufferSize = 0;
		for (auto view : dataMap) {
			minBufferSize = std::max(minBufferSize, view->size());
		}

		// Release all node references in the map for the material properties.
		// They should already have been released individually, but this guarantees
		// that none are missed.
		matToMeshNodes.clear();
	}

	void readView(View* v, char* buffer)
	{
		if (auto view = dynamic_cast<VerticesView*>(v))
		{
			auto vertices64 = view->vector(buffer);
			std::vector<repo::lib::RepoVector3D> vertices32(vertices64.size());
			for (auto i = 0; i < vertices32.size(); i++) {
				vertices32[i] = vertices64[i] - offset;
			}
			view->node->setVertices(vertices32, true);
		}
		else if (auto view = dynamic_cast<IndicesView*>(v))
		{
			auto indices = view->vector(buffer);
			std::vector<repo::lib::repo_face_t> faces(indices.size() / (int)view->primitive);
			auto idx = 0;
			for (auto& f : faces) {
				for (auto i = 0; i < (int)view->primitive; i++) {
					f.push_back(indices[idx++]);
				}
			}
			view->node->setFaces(faces);
		}
		else if (auto view = dynamic_cast<NormalsView*>(v))
		{
			view->node->setNormals(view->vector(buffer));
		}
		else if (auto view = dynamic_cast<UvsView*>(v))
		{
			view->node->setUVChannel(0, view->vector(buffer));
		}
		else if (auto view = dynamic_cast<TextureView*>(v))
		{
			auto n = repo::core::model::RepoBSONFactory::makeTextureNode(
				view->record.filename,
				buffer,
				view->size(),
				view->record.width,
				view->record.height,
				{}
			);
			auto ids = textureIds[view->record.id];
			n.setUniqueID(ids.uniqueId);
			n.setSharedID(ids.uniqueId);
			addNode(n);
		}
	}

	void readData(std::istream* fin)
	{
		auto buffer = new char[minBufferSize];

		size_t position = 0;
		for (auto view : dataMap)
		{
			auto skip = view->begin - position;
			fin->ignore(skip);
			position += skip;

			fin->read(buffer, view->size());
			position += view->size();

			readView(view, buffer);

			delete view;
		}

		delete[] buffer;

		dataMap.clear();
	}

	/*
	* Gets the bounds of the vertices, in the local space of the .bim, without
	* changing the data map - this is used to get the scene bounds for BIM004
	* and below, which are not set in the nodes.
	*/
	repo::lib::RepoBounds readBoundsFromData(std::istream* fin)
	{
		auto buffer = new char[minBufferSize];
		repo::lib::RepoBounds bounds;

		size_t position = 0;
		for (auto v : dataMap)
		{
			if (auto view = dynamic_cast<VerticesView*>(v)) {
				auto skip = view->begin - position;
				fin->ignore(skip);
				position += skip;

				fin->read(buffer, view->size());
				position += view->size();

				for (auto& v : view->vector(buffer)) {
					bounds.encapsulate(v);
				}
			}
		}

		delete[] buffer;

		return bounds;
	}

	void finalise()
	{
		for (auto& r : references) {
			addParent(r.first, r.second);
		}

		RepoSceneBuilder::finalise();
	}

	bool hasMissingTextures() 
	{
		return RepoSceneBuilder::hasMissingTextures();
	}
};



/*
* Will parse the entire BIM file and store the results in
* temporary datastructures in preperation for scene generation.
* @param filePath
* @param database handler
* @param err
*/
repo::core::model::RepoScene* RepoModelImport::importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& err)
{
	std::string fileName = getFileName(filePath);

	repoInfo << "IMPORT [" << fileName << "]";
	repoInfo << "=== IMPORTING MODEL WITH REPO IMPORTER ===";

	// Reading in file
	std::ifstream finCompressed(filePath, std::ios_base::in | std::ios::binary);
	if (finCompressed)
	{
		auto inbuf = new boost::iostreams::filtering_istream();
		inbuf->push(boost::iostreams::gzip_decompressor());
		inbuf->push(finCompressed);

		// Check the BIM file format version
		char fileVersion[REPO_VERSION_LENGTH + 1] = { 0 };
		inbuf->read(fileVersion, REPO_VERSION_LENGTH);
		std::string incomingVersion = fileVersion;
		uint8_t incomingVersionNo = std::stoi(incomingVersion.substr(3, 3));
		if (FILE_META_BYTE_LEN_BY_VERSION.find(incomingVersionNo) == FILE_META_BYTE_LEN_BY_VERSION.end())
		{
			repoError << "Unsupported BIM file version: " << fileVersion;
			err = REPOERR_UNSUPPORTED_BIM_VERSION;
			return nullptr;
		}

		// Loading file metadata
		repoInfo << "Loading BIM file [VERSION: " << incomingVersion << "]";
		size_t metaSize = FILE_META_BYTE_LEN_BY_VERSION.at(incomingVersionNo);
		inbuf->read((char*)&file_meta, metaSize);

		repoInfo << std::left << std::setw(30) << "File meta size: " << metaSize;
		repoInfo << std::left << std::setw(30) << "JSON size: " << file_meta.jsonSize << " bytes";
		repoInfo << std::left << std::setw(30) << "Data buffer size: " << file_meta.dataSize << " bytes";
		repoInfo << std::left << std::setw(30) << "\"sizes\" array start location: " << file_meta.sizesStart << " bytes";
		repoInfo << std::left << std::setw(30) << "\"sizes\" array size: " << file_meta.sizesSize << " bytes";
		repoInfo << std::left << std::setw(30) << "\"materials\" array location: " << file_meta.matStart << " bytes";
		repoInfo << std::left << std::setw(30) << "\"materials\" array size: " << file_meta.matSize << " bytes";
		repoInfo << std::left << std::setw(30) << "\"textures\" array location: " << file_meta.textureSize << " bytes";
		repoInfo << std::left << std::setw(30) << "\"textures\" array size: " << file_meta.textureStart << " bytes";
		repoInfo << std::left << std::setw(30) << "Number of parts to process:" << file_meta.numChildren;

		auto builder = new Builder(
			handler,
			settings.getDatabaseName(),
			settings.getProjectName(),
			settings.getRevisionId()
		);

		repoInfo << "Reading Json header...";

		// Stream in Json tree

		char* jsonBuf = new char[file_meta.jsonSize + 1];
		inbuf->read(jsonBuf, file_meta.jsonSize);
		jsonBuf[file_meta.jsonSize] = '\0';

		TreeParser::ParseJson(jsonBuf, builder);

		builder->prepareDataMap();
		
		repoInfo << "Pre-processing scene bounds (BIM004 and below)...";

		// Get offset

		// In BIM004 and below, the bounds are not set, meaning we need to get
		// them from the geometry directly. We do this by reading the data section
		// once then resetting the stream.

		// For BIM005, we should introduce instancing, and along with that, the
		// primary offset applied at the root node transform.
		// https://github.com/3drepo/3D-Repo-Product-Team/issues/794

		auto bounds = builder->readBoundsFromData(inbuf);
		builder->offset = bounds.min();

		// Now reset the file stream for the actual data read

		delete inbuf;

		finCompressed.seekg(0, std::ios::beg);
		
		inbuf = new boost::iostreams::filtering_istream();
		inbuf->push(boost::iostreams::gzip_decompressor());
		inbuf->push(finCompressed);

		inbuf->ignore(REPO_VERSION_LENGTH);
		inbuf->ignore(metaSize);
		inbuf->ignore(file_meta.jsonSize);

		repoInfo << "Reading data buffer...";

		builder->readData(inbuf);

		delete inbuf;

		repoInfo << "Create scene";

		builder->finalise();

		scene = new repo::core::model::RepoScene(
			settings.getDatabaseName(),
			settings.getProjectName()
		);
		scene->setWorldOffset(builder->offset);

		scene->setRevision(settings.getRevisionId());
		scene->setOriginalFiles({ filePath });
		scene->loadRootNode(handler.get());
		if (builder->hasMissingTextures()) {
			scene->setMissingTexture();
		}

		if (scene->isMissingTexture()){
			err = REPOERR_LOAD_SCENE_MISSING_TEXTURE;
		}

		delete builder;

		return scene;
	}
	else {
		repoError << "File " << fileName << " not found.";
		err = REPOERR_MODEL_FILE_READ;
		return nullptr;
	}
}