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
#include <boost/filesystem.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "repo/core/model/bson/repo_bson_factory.h"
#include <repo_log.h>
#include "repo/error_codes.h"

using namespace repo::core::model;
using namespace repo::manipulator::modelconvertor;
using namespace boost::property_tree;

RepoModelImport::RepoModelImport(const ModelImportConfig& settings) :
	AbstractModelImport(settings)
{
	fin = nullptr;
	finCompressed = nullptr;
}

RepoModelImport::~RepoModelImport()
{
}

repo::core::model::MetadataNode* RepoModelImport::createMetadataNode(
	const ptree& metaTree,
	const std::string& parentName,
	const repo::lib::RepoUUID& parentID)
{
	std::vector<std::string> keys;
	std::vector<repo::lib::RepoVariant> values;

	for (ptree::const_iterator props = metaTree.begin(); props != metaTree.end(); props++)
	{
		std::string origKey = props->first;
		std::string key;
		char type = origKey[0];
		key = origKey.substr(1);

		keys.push_back(key);
		repo::lib::RepoVariant value;
		switch (type)
		{
		case REPO_IMPORT_TYPE_BOOL:
			value = props->second.get_value<bool>();
			break;

		case REPO_IMPORT_TYPE_INT:
			value = props->second.get_value<int>();
			break;

		case REPO_IMPORT_TYPE_DOUBLE:
			value = props->second.get_value<double>();
			break;

		case REPO_IMPORT_TYPE_STRING:
			value = props->second.get_value<std::string>();
			break;
		}
		values.push_back(value);
	}

	repo::core::model::MetadataNode* metaNode = new repo::core::model::MetadataNode(
		repo::core::model::RepoBSONFactory::makeMetaDataNode(keys, values, parentName));

	metadata.insert(metaNode);

	return metaNode;
}

void RepoModelImport::parseMaterial(const boost::property_tree::ptree& matTree)
{
	repo::lib::repo_material_t repo_material;
	int textureId = -1;

	if (matTree.find("diffuse") != matTree.not_found())
		repo_material.diffuse = as_vector<float>(matTree, "diffuse");

	if (matTree.find("specular") != matTree.not_found())
		repo_material.specular = as_vector<float>(matTree, "specular");

	if (matTree.find("emissive") != matTree.not_found())
		repo_material.emissive = as_vector<float>(matTree, "emissive");

	if (matTree.find("ambient") != matTree.not_found())
		repo_material.ambient = as_vector<float>(matTree, "ambient");

	if (matTree.find("transparency") != matTree.not_found())
		repo_material.opacity = 1.0f - matTree.get<float>("transparency");
	else
		repo_material.opacity = 1.0f;

	if (matTree.find("shininess") != matTree.not_found())
		repo_material.shininess = matTree.get<float>("shininess") * 5;
	else
		repo_material.shininess = 0.0f;

	if (matTree.find("texture") != matTree.not_found())
	{
		textureId = matTree.get<int>("texture");
	}

	if (matTree.find("lineWeight") != matTree.not_found())
	{
		repo_material.lineWeight = matTree.get<float>("lineWeight");
	}
	else
	{
		repo_material.lineWeight = std::numeric_limits<float>::quiet_NaN();
	}

	repo_material.shininessStrength = 0.25f;
	repo_material.isWireframe = false;
	repo_material.isTwoSided = false;

	std::stringstream ss("");
	ss << "Material." << std::setfill('0') << std::setw(5) << materials.size();

	repo::core::model::MaterialNode* materialNode = new repo::core::model::MaterialNode(
		repo::core::model::RepoBSONFactory::makeMaterialNode(repo_material, ss.str()));
	materials.insert(materialNode);
	matNodeList.push_back(materialNode);

	if (textureId >= 0)
	{
		textureIdToParents[textureId].push_back(materialNode->getSharedID());
	}
}

void RepoModelImport::parseTexture(
	const boost::property_tree::ptree& textureTree,
	char* const dataBuffer)
{
	bool fileNameOk = textureTree.find(REPO_TXTR_FNAME) != textureTree.not_found();
	bool byteCountOK = textureTree.find(REPO_TXTR_NUM_BYTES) != textureTree.not_found();
	bool widthOk = textureTree.find(REPO_TXTR_WIDTH) != textureTree.not_found();
	bool heightOk = textureTree.find(REPO_TXTR_HEIGHT) != textureTree.not_found();
	bool idOk = textureTree.find(REPO_TXTR_ID) != textureTree.not_found();

	// do the fields exist
	if (!byteCountOK ||
		!widthOk ||
		!heightOk ||
		!idOk)
	{
		repoError << "Required texture field missing. Skipping this texture.";
		missingTextures = true;
		return;
	}

	std::string name = fileNameOk ? textureTree.get_child(REPO_TXTR_FNAME).data() : "";
	uint32_t byteCount = textureTree.get<uint32_t>(REPO_TXTR_NUM_BYTES);
	uint32_t width = textureTree.get<uint32_t>(REPO_TXTR_WIDTH);
	uint32_t height = textureTree.get<uint32_t>(REPO_TXTR_HEIGHT);
	uint32_t id = textureTree.get<uint32_t>(REPO_TXTR_ID);

	// are they valid
	if (byteCount == 0)
	{
		repoError << "No data buffer size for the texture " << name;
		missingTextures = true;
		return;
	}

	std::vector<uint64_t> DataStartEnd = as_vector<uint64_t>(textureTree, REPO_TXTR_IMG_BYTES);

	char* data = &dataBuffer[DataStartEnd[0]];

	repo::core::model::TextureNode* textureNode =
		new repo::core::model::TextureNode(
			repo::core::model::RepoBSONFactory::makeTextureNode(
				name,
				data,
				byteCount,
				width,
				height,
				textureIdToParents[id]));

	textures.insert(textureNode);
}

RepoModelImport::mesh_data_t RepoModelImport::createMeshRecord(
	const ptree& mesh,
	const std::string& parentName,
	const repo::lib::RepoUUID& parentID,
	const repo::lib::RepoMatrix& trans)
{
	int materialID = -1;

	auto numIndices = mesh.get<int64_t>("numIndices");
	auto numVertices = mesh.get<int64_t>("numVertices");

	std::vector<repo::lib::RepoVector3D64> vertices;
	std::vector<repo::lib::RepoVector3D> normals;
	std::vector<std::vector<repo::lib::RepoVector2D>> uvChannels;
	std::vector<repo::lib::repo_face_t> faces;

	repo::lib::RepoBounds bounds;

	const bool needTransform = !trans.isIdentity();
	repo::lib::RepoMatrix normalTrans;

	if (needTransform)
	{
		repo::lib::RepoMatrix invTrans;

		invTrans = trans.invert().transpose();

		auto data = invTrans.getData();
		data[3] = data[7] = data[11] = 0;
		data[12] = data[13] = data[14] = 0;

		normalTrans = repo::lib::RepoMatrix(data);
	}

	for (ptree::const_iterator props = mesh.begin(); props != mesh.end(); props++)
	{
		if (props->first == REPO_IMPORT_MATERIAL)
		{
			materialID = props->second.get_value<int>();
		}
		else if (props->first == REPO_IMPORT_VERTICES || props->first == REPO_IMPORT_NORMALS)
		{
			std::vector<int64_t> startEnd = as_vector<int64_t>(mesh, props->first);

			double* tmpVerticesDouble = (double*)(dataBuffer + startEnd[0]);
			float* tmpVerticesSingle = (float*)(dataBuffer + startEnd[0]);

			for (int i = 0; i < numVertices; i++)
			{
				if (props->first == REPO_IMPORT_VERTICES) {
					repo::lib::RepoVector3D64 tmpVec;
					tmpVec = { tmpVerticesDouble[i * 3] ,  tmpVerticesDouble[i * 3 + 1] , tmpVerticesDouble[i * 3 + 2] };
					if (needTransform) tmpVec = trans * tmpVec;

					bounds.encapsulate(tmpVec);

					vertices.push_back(tmpVec);
				}
				else {
					repo::lib::RepoVector3D tmpVec =
					{
						tmpVerticesSingle[i * 3] ,
						tmpVerticesSingle[i * 3 + 1] ,
						tmpVerticesSingle[i * 3 + 2]
					};
					normals.push_back(needTransform ? normalTrans * tmpVec : tmpVec);
				}
			}
		}
		else if (props->first == REPO_IMPORT_UV)
		{
			std::vector<int64_t> startEnd = as_vector<int64_t>(mesh, props->first);
			float* tmpUVs = (float*)(dataBuffer + startEnd[0]);
			std::vector<repo::lib::RepoVector2D> uvChannelVector;
			for (int i = 0; i < numVertices; i++)
			{
				repo::lib::RepoVector2D tmpUVVec = repo::lib::RepoVector2D(tmpUVs[i * 2], tmpUVs[i * 2 + 1]);
				uvChannelVector.push_back(tmpUVVec);
			}
			uvChannels.push_back(uvChannelVector);
		}
		else if (props->first == REPO_IMPORT_INDICES)
		{
			std::vector<int64_t> startEnd = as_vector<int64_t>(mesh, REPO_IMPORT_INDICES);

			uint32_t* tmpIndices = (uint32_t*)(dataBuffer + startEnd[0]);

			// figure out primitive type
			boost::optional<const ptree&> primitiveProp = mesh.get_child_optional(REPO_IMPORT_PRIMITIVE);
			MeshNode::Primitive primitiveType;
			if (primitiveProp)
			{
				primitiveType = static_cast<MeshNode::Primitive>(primitiveProp.get().get_value<int8_t>());
			}
			else
			{
				primitiveType = MeshNode::Primitive::TRIANGLES;
			}

			// pull out faces
			const auto primitiveIdxLen = (int8_t)primitiveType;
			switch (primitiveType)
			{
			case repo::core::model::MeshNode::Primitive::LINES:
			case repo::core::model::MeshNode::Primitive::TRIANGLES:
				for (int i = 0; i < numIndices; i += primitiveIdxLen)
				{
					repo::lib::repo_face_t tmpFace;
					for (int j = 0; j < primitiveIdxLen; ++j)
					{
						tmpFace.push_back(tmpIndices[i + j]);
					}
					faces.push_back(tmpFace);
				}
				break;
			default:
				geometryImportError = true;
				break;
			}
		}
	}

	if (offset) {
		offset = repo::lib::RepoVector3D64::min(offset.value(), bounds.min());
	}
	else {
		offset = bounds.min();
	}

	const auto sharedID = repo::lib::RepoUUID::createUUID();

	if (materialID >= 0)
	{
		matNodeList[materialID]->addParent(sharedID);
	}

	mesh_data_t result = { vertices, normals, uvChannels, faces, bounds, parentID, sharedID };
	return result;
}

void RepoModelImport::createObject(const ptree& tree)
{
	int myID = tree.get<int>("id");
	int myParent = tree.get<int>("parent");

	std::string transName = tree.get<std::string>("name", "");

	repo::lib::RepoUUID parentSharedID = node_map[myParent]->getSharedID();
	repo::lib::RepoMatrix parentTransform = trans_matrix_map[myParent];

	boost::optional< const ptree& > transMatTree = tree.get_child_optional("transformation");

	repo::core::model::TransformationNode* transNode = (repo::core::model::TransformationNode*) node_map[myParent];

	// We only want to create a node if there is a matrix transformation to represent, or
	// we're trying to represent a different entity to its parent. Otherwise, reuse the parent transform and only store the geometry
	bool isEntity = !transName.empty() || transMatTree;

	if (isEntity) {
		if (transMatTree) {
			repo::lib::RepoMatrix transMat = repo::lib::RepoMatrix(as_vector<float>(tree, "transformation"));
			trans_matrix_map.push_back(parentTransform * transMat);
		}
		else {
			trans_matrix_map.push_back(parentTransform);
		}
		transNode =
			new repo::core::model::TransformationNode(
				repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), transName, { parentSharedID }));
		transformations.insert(transNode);
	}
	else {
		trans_matrix_map.push_back(parentTransform);
	}
	node_map.push_back(transNode);

	std::vector<repo::core::model::MetadataNode*> metas;
	std::vector<repo::lib::RepoUUID> metaParentIDs;

	for (ptree::const_iterator props = tree.begin(); props != tree.end(); props++)
	{
		if (isEntity && props->first == REPO_IMPORT_METADATA)
		{
			// The assumption is that if the entry is not an individual entity, we don't want to import the metadata.
			metas.push_back(createMetadataNode(props->second, transName, parentSharedID));
		}

		if (props->first == REPO_IMPORT_GEOMETRY)
		{
			auto mesh = createMeshRecord(props->second, transNode->getName(), transNode->getSharedID(), trans_matrix_map.back());
			metaParentIDs.push_back(mesh.sharedID);
			meshEntries.push_back(mesh);
		}
	}

	if (isEntity) {
		metaParentIDs.push_back(transNode->getSharedID());

		for (auto& meta : metas)
		{
			meta->addParents(metaParentIDs);
		}
	}
}

void RepoModelImport::skipAheadInFile(long amount)
{
	if (amount > 0)
	{
		// Cannot use seekg on GZIP file
		char* tmpBuf = new char[amount];
		fin->read(tmpBuf, amount);
		delete[] tmpBuf;
	}
}

/**
* Will parse the entire BIM file and store the results in
* temporary datastructures in preperation for scene generation.
* @param filePath
* @param err
*/
bool RepoModelImport::importModel(std::string filePath, uint8_t& err)
{
	orgFile = filePath;
	std::string fileName = getFileName(filePath);

	repoInfo << "IMPORT [" << fileName << "]";
	repoInfo << "=== IMPORTING MODEL WITH REPO IMPORTER ===";

	// Reading in file
	finCompressed = new std::ifstream(filePath, std::ios_base::in | std::ios::binary);
	if (finCompressed)
	{
		inbuf = new boost::iostreams::filtering_streambuf<boost::iostreams::input>();
#ifndef REPO_BOOST_NO_GZIP
		inbuf->push(boost::iostreams::gzip_decompressor());
#else
		repoWarning << "Gzip is not compiled into Boost library, .bim imports may not work as intended";
#endif
		inbuf->push(*finCompressed);
		fin = new std::istream(inbuf);

		// Check the BIM file format version
		char fileVersion[REPO_VERSION_LENGTH + 1] = { 0 };
		fin->read(fileVersion, REPO_VERSION_LENGTH);
		std::string incomingVersion = fileVersion;
		uint8_t incomingVersionNo = std::stoi(incomingVersion.substr(3, 3));
		if (FILE_META_BYTE_LEN_BY_VERSION.find(incomingVersionNo) == FILE_META_BYTE_LEN_BY_VERSION.end())
		{
			repoError << "Unsupported BIM file version: " << fileVersion;
			err = REPOERR_UNSUPPORTED_BIM_VERSION;
			return false;
		}

		// Loading file metadata
		repoInfo << "Loading BIM file [VERSION: " << incomingVersion << "]";
		size_t metaSize = REPO_VERSION_LENGTH + sizeof(fileMeta);
		fin->read((char*)&file_meta, FILE_META_BYTE_LEN_BY_VERSION.at(incomingVersionNo));

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

		// Load full JSON tree
		boost::property_tree::ptree jsonRoot = getNextJSON(file_meta.jsonSize);

		// Load binary data
		repoInfo << "Reading data buffer";
		dataBuffer = new char[file_meta.dataSize];
		fin->read(dataBuffer, file_meta.dataSize);

		// Loading in required JSON nodes
		boost::optional<ptree&> materialsRoot = jsonRoot.get_child_optional("materials");
		if (materialsRoot)
		{
			for (ptree::value_type element : materialsRoot.get())
			{
				parseMaterial(element.second);
			}
			repoInfo << "Loaded: " << materials.size() << " materials";
		}
		else
		{
			repoError << "File " << fileName << " does not have a \"materials\" node";
			err = REPOERR_MODEL_FILE_READ;
			return false;
		}
		boost::optional<ptree&> sizesRoot = jsonRoot.get_child_optional("sizes");
		if (sizesRoot)
		{
			sizes = as_vector<long>(sizesRoot.get());
		}
		else
		{
			repoError << "File " << fileName << " does not have a \"sizes\" node";
			err = REPOERR_MODEL_FILE_READ;
			return false;
		}
		boost::optional<ptree&> texturesRoot = jsonRoot.get_child_optional("textures");
		if (texturesRoot)
		{
			for (ptree::value_type element : texturesRoot.get())
			{
				parseTexture(element.second, dataBuffer);
			}
			repoInfo << "Loaded: " << textures.size() << " textures";
			if (textureIdToParents.size() > 0)
			{
				int maxTextureId = textureIdToParents.rbegin()->first;
				// Texture ids in the material JSON should map
				// directly on to the order they appear in the texture JSON
				if (maxTextureId > (textures.size() - 1))
				{
					repoError << "A material is referencing a missing texture";
					missingTextures = true;
				}
			}
		}

		if (missingTextures)
		{
			err = REPOERR_LOAD_SCENE_MISSING_TEXTURE;
		}

		// Clean up
		finCompressed->close();
		delete finCompressed;
		finCompressed = new std::ifstream(filePath, std::ios_base::in | std::ios::binary);
		delete fin;
		delete inbuf;

		// Reloading file, skipping to the root start. No seekg for GZIPped files.
		inbuf = new boost::iostreams::filtering_streambuf<boost::iostreams::input>();
#ifndef REPO_BOOST_NO_GZIP
		inbuf->push(boost::iostreams::gzip_decompressor());
#endif
		inbuf->push(*finCompressed);
		fin = new std::istream(inbuf);
		skipAheadInFile(sizes[0]);

		return true;
	}
	else {
		repoError << "File " << fileName << " not found.";
		err = REPOERR_MODEL_FILE_READ;
		return false;
	}
}

boost::property_tree::ptree RepoModelImport::getNextJSON(long jsonSize)
{
	boost::property_tree::ptree singleJSON;
	char* jsonBuf = new char[jsonSize + 1];

	fin->read(jsonBuf, jsonSize);
	jsonBuf[jsonSize] = '\0';

	std::stringstream bufReader(jsonBuf);
	read_json(bufReader, singleJSON);

	delete[] jsonBuf;

	return singleJSON;
}

repo::core::model::RepoScene* RepoModelImport::generateRepoScene(uint8_t& errCode)
{
	repoInfo << "Generating scene";

	// Process root node
	boost::property_tree::ptree root = getNextJSON(sizes[1]);
	std::string rootName = root.get<std::string>("name", "");
	boost::optional< ptree& > rootBBOX = root.get_child_optional("bbox");
	if (!rootBBOX) {
		repoError << "No root bounding box specified.";
		errCode = REPOERR_MODEL_FILE_READ;
		return nullptr;
	}
	boost::optional< ptree& > transMatTree = root.get_child_optional("transformation");
	repo::lib::RepoMatrix transMat;
	if (transMatTree)
	{
		transMat = repo::lib::RepoMatrix(as_vector<float>(root, "transformation"));
	}
	repo::core::model::TransformationNode* rootNode =
		new repo::core::model::TransformationNode(
			repo::core::model::RepoBSONFactory::makeTransformationNode(
				repo::lib::RepoMatrix(), rootName, std::vector<repo::lib::RepoUUID>()));
	node_map.push_back(rootNode);
	trans_matrix_map.push_back(transMat);
	transformations.insert(rootNode);

	// Process children of root node
	char comma;
	for (long i = 0; i < file_meta.numChildren; i++)
	{
		if (i % 500 == 0 || i == file_meta.numChildren - 1)
		{
			repoInfo << "Importing " << i << " of " << file_meta.numChildren << " JSON nodes";
		}
		fin->read(&comma, 1);
		boost::property_tree::ptree jsonTree = getNextJSON(sizes[i + 2]);
		createObject(jsonTree);
	}

	// Change the container type of the materials for RepoScene's constructor
	repoInfo << "Building materials list";
	materials.clear();
	for (int i = 0; i < matNodeList.size(); i++)
	{
		materials.insert(matNodeList[i]);
	}

	// Preparing reference files
	std::vector<std::string> fileVect;
	if (!orgFile.empty())
	{
		fileVect.push_back(orgFile);
	}

	// Processing meshes
	repo::core::model::RepoNodeSet meshes;
	if (!offset) offset = repo::lib::RepoVector3D64(0, 0, 0);
	for (const auto& entry : meshEntries) {
		std::vector<repo::lib::RepoVector3D> vertices;
		// Offsetting all the verts by the world offset to reduce the magnitude
		// of their values so they can be cast to floats (widely used in 3D libs)
		for (const auto& v : entry.rawVertices) {
			repo::lib::RepoVector3D v32 = (repo::lib::RepoVector3D)(v - offset.value());
			vertices.push_back(v32);
		}

		repo::lib::RepoBounds boundingBox(
			entry.boundingBox.min() - offset.value(),
			entry.boundingBox.max() - offset.value()
		);

		auto mesh = repo::core::model::RepoBSONFactory::makeMeshNode(
			vertices,
			entry.faces,
			entry.normals,
			boundingBox,
			entry.uvChannels,
			std::string(),
			{ entry.parent });
		mesh.setSharedID(entry.sharedID);
		meshes.insert(new repo::core::model::MeshNode(mesh));
	}

	// Generate scene
	repo::core::model::RepoScene* scenePtr = new repo::core::model::RepoScene(
		fileVect, meshes, materials, metadata, textures, transformations);
	scenePtr->setWorldOffset({ offset.value().x, offset.value().y, offset.value().z });

	// Error handling
	if (missingTextures)
	{
		errCode = REPOERR_LOAD_SCENE_MISSING_TEXTURE;
		scenePtr->setMissingTexture();
	}
	if (geometryImportError)
	{
		repoError << "Unsupported geometry primitive type found	";
		errCode = REPOERR_GEOMETRY_ERROR;
	}

	// Cleanup
	delete[] dataBuffer;
	finCompressed->close();
	delete fin;
	delete inbuf;
	delete finCompressed;

	return scenePtr;
}