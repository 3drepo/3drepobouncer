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
#include <stack>
#include <iostream>
#include <istream>
#include <ranges>
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
#include "rapidjson/reader.h"

#pragma optimize("", off)

using namespace repo::core::model;
using namespace repo::manipulator::modelconvertor;
using namespace rapidjson;

const char REPO_IMPORT_TYPE_STRING = 'S';
const char REPO_IMPORT_TYPE_DOUBLE = 'D';
const char REPO_IMPORT_TYPE_INT = 'I';
const char REPO_IMPORT_TYPE_BOOL = 'B';
const char REPO_IMPORT_TYPE_DATETIME = 'T';

const static int REPO_VERSION_LENGTH = 6;

RepoModelImport::RepoModelImport(const ModelImportConfig& settings) :
	AbstractModelImport(settings)
{
	scene = nullptr;
}

RepoModelImport::~RepoModelImport()
{
}

bool RepoModelImport::importModel(std::string filePath, uint8_t& errMsg)
{
	throw repo::lib::RepoException("Classic import is no longer supported for .bim. Please use the streaming import.");
}

struct Range
{
	size_t start;
	size_t end;

	Range()
		:start(0),
		end(0)
	{
	}

	size_t size() const
	{
		return end - start;
	}
};

struct TextureRecord
{
	std::string filename;
	int numBytes;
	int width;
	int height;
	int id;
	Range bytes;

	TextureRecord() :
		numBytes(0),
		width(0),
		height(0),
		id(-1)
	{
	}

	bool isOK() const
	{
		return bytes.size() > 0 && width > 0 && height > 0;
	}
};

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

	std::vector<T> vector(char* data)
	{
		std::vector<T> vector;
		vector.resize(size() / sizeof(T));
		memcpy_s(vector.data(), size(), data, size());
		return vector;
	}
};

struct VerticesView : public MeshAttributeView<repo::lib::RepoVector3D64>
{
	using MeshAttributeView::MeshAttributeView;
};

struct NormalsView : public MeshAttributeView<repo::lib::RepoVector3D>
{
	using MeshAttributeView::MeshAttributeView;
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

struct GeometryRecord
{
	repo::core::model::MeshNode::Primitive primitive;
	size_t numIndices;
	size_t numVertices;
	Range indices;
	Range vertices;
	Range normals;
	Range uv;
	int material;
	bool hasGeometry;

	GeometryRecord()
		:primitive(repo::core::model::MeshNode::Primitive::TRIANGLES), // Default to Triangles for compatibility with earlier versions
		numIndices(0),
		numVertices(0),
		material(0),
		hasGeometry(false)
	{
	}
};

struct NodeRecord
{
	int id;
	int parentId;
	std::string name;
	repo::lib::RepoMatrix64 matrix;
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
	GeometryRecord geometry;
	repo::lib::RepoBounds bounds;

	NodeRecord()
		:id(-1),
		parentId(-1)
	{
	}
};

/*
* A subclass of RepoSceneBuilder with a dictionary for mapping local indices.
*/
class Builder : private repo::manipulator::modelutility::RepoSceneBuilder
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
	std::unordered_map<int, Ids> materialIds;
	std::unordered_map<int, Ids> textureIds;
	size_t numMaterials;
	std::vector<View*> dataMap;
	size_t minBufferSize;
	repo::lib::RepoVector3D64 offset; // Applied to the vertex data
	repo::lib::RepoMatrix64 rootTransform; // For BIM004 and below, there is one (root) transform that is applied to all geometry

	/* 
	* Used to set the parents once all nodes have been read in. This is used
	* instead of RepoSceneBuilder's addParent until the end, because the Ids may
	* be created before the actual nodes themselves.
	*/
	std::vector<std::pair<repo::lib::RepoUUID, repo::lib::RepoUUID>> references; // From uniqueId to sharedId

	void getParent(int id, std::vector<repo::lib::RepoUUID>& parents)
	{
		auto it = transformSharedIds.find(id);
		if (it != transformSharedIds.end())
		{
			parents.push_back(it->second);
		}
	}

	void createNode(const NodeRecord& r)
	{
		std::vector<repo::lib::RepoUUID> parentIds;
		getParent(r.parentId, parentIds);

		if (parentIds.empty()) {
			rootTransform = r.matrix;
		}

		bool isEntity = !r.name.empty() || !r.matrix.isIdentity();
		if (isEntity)
		{
			auto node = repo::core::model::RepoBSONFactory::makeTransformationNode(
				(repo::lib::RepoMatrix)r.matrix, // When this downcast is performed the world offset should have already been removed from the root
				r.name,
				parentIds
			);
			transformSharedIds[r.id] = node.getSharedID();
			parentIds.clear();
			parentIds.push_back(node.getSharedID());
			addNode(node);
		}

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

			if (r.geometry.vertices.size()) {
				dataMap.push_back(new VerticesView(r.geometry.vertices, node));
			}

			if (r.geometry.indices.size()) {
				dataMap.push_back(new IndicesView(r.geometry.indices, node, r.geometry.primitive));
			}

			if (r.geometry.normals.size()) {
				dataMap.push_back(new NormalsView(r.geometry.normals, node));
			}

			if (r.geometry.uv.size()) {
				dataMap.push_back(new UvsView(r.geometry.uv, node));
			}

			auto& material = materialIds[r.geometry.material];
			references.push_back({ material.uniqueId, node->getSharedID() });
		}

		if (r.metadata.size())
		{
			auto node = repo::core::model::RepoBSONFactory::makeMetaDataNode(r.metadata, {}, parentIds);
			addNode(node);
		}
	}

	// It is expected that materials are added in the order they are indexed by
	// the nodes
	void createMaterial(repo::lib::repo_material_t material, int texture)
	{
		auto n = repo::core::model::RepoBSONFactory::makeMaterialNode(material, {}, {});
		auto& ids = materialIds[numMaterials++];
		n.setUniqueID(ids.uniqueId);
		n.setSharedID(ids.sharedId);
		addNode(n);

		if (texture != -1) {
			references.push_back({ textureIds[texture].uniqueId, ids.sharedId });
		}
	}

	void createTexture(const TextureRecord& t)
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
* Parsers map primitives from the Json to members of more complex structures.
* 
* Parsers typically hold a reference to the variable they are meant to update,
* and when they receive the value via one of the base methods, they can perform
* any necessary post-processing, and assign it.
* 
* Parser objects can also respond to object & array delimiters and keys; if a
* parser returns another Parser pointer, the new Parser will be added to the
* stack, and used for any subsequent values/tokens, until the scope (object or
* array) exits. In this way a tree of Parser objects can be constructed
* to handle complex types with multiple nested types, such as Nodes or
* Bounding Boxes.
* 
* It is expected the Parser tree is created once per-application, and the
* memory sections that it updates are written to over and over as new objects
* are read from the Json.
*/

struct Parser
{
	virtual void Null() {}
	virtual void Bool(bool b) {}
	virtual void Int(int i) {}
	virtual void Uint(unsigned i) {}
	virtual void Int64(int64_t i) {}
	virtual void Uint64(uint64_t i) {}
	virtual void Double(double d) {}
	virtual void String(const std::string_view& string) {}
	virtual void StartObject() {}
	virtual Parser* Key(const std::string_view& key) { return nullptr;  }
	virtual void EndObject() {}
	virtual Parser* StartArray() { return nullptr; }
	virtual void EndArray() {}
};

/*
* Calls the parser at the top of the stack for any value. When arrays or objects
* are encountered, the Parser has the opportunity to push a new parser to the
* stack for the duration of that scope.
* Objects are additionally special in that they have two stack entries: one
* for the object Parser itself, and one for the Parser for each key. When
* key is called inside an Object, the top of the stack is *swapped* for the new
* Parser. When the object terminates, the top Parser is discarded and EndObject
* called on the Parser on which StartObject was called.
*/

struct MyHandler : public BaseReaderHandler<UTF8<>, MyHandler>
{
	bool StartObject() {
		if (stack.top()) {
			stack.top()->StartObject();
		}
		stack.push(nullptr); // This one is to hold the Parsers for the objects' members. This should be updated after the call to Key before any values are read.
		return true;
	}

	bool Key(const Ch* str, SizeType length, bool copy) {
		stack.pop();
		stack.push(stack.top() ? stack.top()->Key(std::string_view(str, length)) : nullptr);
		return true;
	}
	
	bool EndObject(SizeType) {
		stack.pop(); // (The key's parser, or nullptr if one was never set.)
		if (stack.top()) {
			stack.top()->EndObject();
		}
		return true;
	}

	bool StartArray() {
		stack.push(stack.top() ? stack.top()->StartArray() : nullptr);
		return true;
	}

	bool EndArray(SizeType) { 
		stack.pop();
		if (stack.top()) {
			stack.top()->EndArray();
		}
		return true;
	}

	bool String(const Ch* str, SizeType length, bool copy) {
		if (stack.top()) {
			stack.top()->String(std::string_view(str, length));
		}
		return true;
	}

	bool Bool(bool b) {
		if (stack.top()) {
			stack.top()->Bool(b);
		}
		return true;
	}

	bool Int(int v) {
		if (stack.top()) {
			stack.top()->Int(v);
		}
		return true;
	}

	bool Uint(unsigned v) {
		if (stack.top()) {
			stack.top()->Uint(v);
		}
		return true;
	}

	bool Int64(int64_t v) {
		if (stack.top()) {
			stack.top()->Int64(v);
		}
		return true;
	}

	bool Uint64(uint64_t v) {
		if (stack.top()) {
			stack.top()->Uint64(v);
		}
		return true;
	}

	bool Double(double v) {
		if (stack.top()) {
			stack.top()->Double(v);
		}
		return true;
	}

	std::stack<Parser*> stack;
};


struct ObjectParser : public Parser
{
	std::map<std::string, Parser*, std::less<>> parsers;

	virtual Parser* Key(const std::string_view& key) override {
		auto it = parsers.find(key);
		if (it != parsers.end()) {
			return it->second;
		}
		else {
			return nullptr;
		}
	}
};

/*
* Tells the handler to call elementParser for each element of the array
*/
struct ArrayParser : public Parser
{
	Parser* elementParser;

	ArrayParser(Parser* elementParser)
		:elementParser(elementParser) {
	}

	virtual Parser* StartArray() {
		return elementParser;
	}
};

/*
* Casts any number received to the desired type and outputs it via the single
* abstract method.
*/
template<typename T>
struct NumberParserBase : public Parser
{
	virtual void Int(int i) override {
		Number((T)i);
	}

	virtual void Uint(unsigned i) override {
		Number((T)i);
	}

	virtual void Int64(int64_t i) override {
		Number((T)i);
	}

	virtual void Uint64(uint64_t i) override {
		Number((T)i);
	}

	virtual void Double(double d) override {
		Number((T)d);
	}

	virtual void Number(T v) = 0;
};

template<typename T>
struct NumberParser : public NumberParserBase<T>
{
	T& v;

	NumberParser(T& v)
		:v(v)
	{
	}

	virtual void Number(T v) override {
		this->v = v;
	}
};

struct StringParser : public Parser
{
	std::string& v;

	StringParser(std::string& v)
		:v(v)
	{
	}

	virtual void String(const std::string_view& s) override {
		v = s;
	}
};

// Reads an array as a RepoVector
struct VectorParser : public NumberParserBase<double>
{
	repo::lib::RepoVector3D64& vec;
	int i;

	VectorParser(repo::lib::RepoVector3D64& vec)
		:i(0),
		vec(vec)
	{
	}

	virtual Parser* StartArray() override {
		vec = repo::lib::RepoVector3D64();
		i = 0;
		return this;
	}

	virtual void Number(double d) override {
		if (i < 3) {
			((double*)&vec)[i++] = d;
		}
	}
};

struct BoundsParser : public ObjectParser
{
	repo::lib::RepoVector3D64 min;
	repo::lib::RepoVector3D64 max;

	repo::lib::RepoBounds& bounds;

	BoundsParser(repo::lib::RepoBounds& bounds):
		bounds(bounds)
	{
		parsers["min"] = new VectorParser(min);
		parsers["max"] = new VectorParser(max);
	}

	void EndObject() override {
		bounds = repo::lib::RepoBounds(min, max);
	}
};

struct RepoVariantParser : public Parser
{
	repo::lib::RepoVariant* v;

	RepoVariantParser() {
		v = nullptr;
	}

	virtual void Bool(bool b) { *v = b; }
	virtual void Int(int i) { *v = i; }
	virtual void Uint(unsigned i) { *v = (int)i; }
	virtual void Int64(int64_t i) { *v = i; }
	virtual void Uint64(uint64_t i) { *v = (int64_t)i; }
	virtual void Double(double d) { *v = d; }
	virtual void String(const std::string_view& string) { *v = std::string(string); }
};

struct MetadataParser : public Parser
{
	std::unordered_map<std::string, repo::lib::RepoVariant>& metadata;
	RepoVariantParser valueParser;

	MetadataParser(std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
		:metadata(metadata)
	{
	}

	Parser* Key(const std::string_view& key) override {
		valueParser.v = &metadata[std::string(key).substr(1)]; // BIM004 and below prefix metadata keys with a character indicating the type
		return &valueParser;
	}

	void StartObject() override {
		metadata = {};
	}
};

struct RepoMatrixParser : public NumberParserBase<float>
{
	repo::lib::RepoMatrix64& matrix;
	int i;

	RepoMatrixParser(repo::lib::RepoMatrix64& matrix)
		:matrix(matrix),
		i(0)
	{
	}

	virtual Parser* StartArray() {
		matrix = repo::lib::RepoMatrix64();
		i = 0;
		return this;
	}

	void Number(float d) override {
		if (i < matrix.getData().size()) {
			matrix.getData()[i++] = d;
		}
	}
};

struct RangeParser : public NumberParserBase<size_t>
{
	bool started = false;

	Range& range;

	RangeParser(Range& range):
		range(range)
	{
	}

	virtual Parser* StartArray() {
		started = false;
		return this;
	}

	void Number(size_t n) override {
		if (!started) {
			range.start = n;
			started = true;
		}
		else {
			range.end = n;
		}
	}
};

struct ColourParser : public NumberParserBase<float>
{
	int i;
	repo::lib::repo_color3d_t& colour;

	ColourParser(repo::lib::repo_color3d_t& colour) :
		colour(colour),
		i(0)
	{}

	virtual Parser* StartArray() {
		i = 0;
		return this;
	}

	virtual void Number(float d) override 	{
		if (i < 3) {
			((float*)&colour)[i++] = d;
		}
	}
};

struct TextureParser : public ObjectParser
{
	TextureRecord texture;
	Builder* builder;

	TextureParser(Builder* builder)
		:builder(builder)
	{
		parsers["id"] = new NumberParser<int>(texture.id);
		parsers["filename"] = new StringParser(texture.filename);
		parsers["width"] = new NumberParser<int>(texture.width);
		parsers["height"] = new NumberParser<int>(texture.height);
		parsers["imageBytes"] = new RangeParser(texture.bytes);
	}

	virtual void StartObject() override {
		texture = TextureRecord();
	}

	virtual void EndObject() override {
		builder->createTexture(texture);
	}
};

struct MaterialParser : public ObjectParser
{
	repo::lib::repo_material_t material;
	int texture;
	Builder* builder;

	MaterialParser(Builder* builder)
		:builder(builder)
	{
		parsers["diffuse"] = new ColourParser(material.diffuse);
		parsers["ambient"] = new ColourParser(material.ambient);
		parsers["specular"] = new ColourParser(material.specular);
		parsers["emissive"] = new ColourParser(material.emissive);
		parsers["transparency"] = new NumberParser<float>(material.opacity);
		parsers["shininess"] = new NumberParser<float>(material.shininess);
		parsers["lineWeight"] = new NumberParser<float>(material.lineWeight);
		parsers["texture"] = new NumberParser<int>(texture);
	}

	virtual void StartObject() override {
		material = repo::lib::repo_material_t::DefaultMaterial();
		material.opacity = 0;
		material.shininess = material.shininess / 5;
		texture = -1;
	}

	virtual void EndObject() override {
		material.opacity = 1 - material.opacity;
		material.shininess = material.shininess * 5;
		builder->createMaterial(material, texture);
	}
};

struct GeometryParser : public ObjectParser
{
	GeometryRecord& geometry;

	GeometryParser(GeometryRecord& geometry)
		:geometry(geometry)
	{
		parsers["primitive"] = new NumberParser(geometry.primitive);
		parsers["numIndices"] = new NumberParser(geometry.numIndices);
		parsers["numVertices"] = new NumberParser(geometry.numVertices);
		parsers["indices"] = new RangeParser(geometry.indices);
		parsers["vertices"] = new RangeParser(geometry.vertices);
		parsers["normals"] = new RangeParser(geometry.normals);
		parsers["uv"] = new RangeParser(geometry.uv);
		parsers["material"] = new NumberParser(geometry.material);
	}

	virtual void StartObject() {
		geometry.hasGeometry = true;
	}
};

struct NodeParser : public ObjectParser
{
	NodeRecord record;
	Builder* builder;

	NodeParser(Builder* builder)
		:builder(builder)
	{
		parsers["id"] = new NumberParser(record.id);
		parsers["name"] = new StringParser(record.name);
		parsers["parent"] = new NumberParser(record.parentId);
		parsers["metadata"] = new MetadataParser(record.metadata);
		parsers["geometry"] = new GeometryParser(record.geometry);
		parsers["bounds"] = new BoundsParser(record.bounds);
		parsers["transformation"] = new RepoMatrixParser(record.matrix);
	}

	virtual void StartObject() {
		record = NodeRecord();
	}

	virtual void EndObject() {
		builder->createNode(record);
	}
};

struct TreeParser : public ObjectParser
{
	TreeParser(Builder* builder)
	{
		parsers["nodes"] = new ArrayParser(new NodeParser(builder));
		parsers["materials"] = new ArrayParser(new MaterialParser(builder));
		parsers["textures"] = new ArrayParser(new TextureParser(builder));
	}
};

/*
* Will parse the entire BIM file and store the results in
* temporary datastructures in preperation for scene generation.
* @param filePath
* @param err
*/
bool RepoModelImport::importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& err)
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
			return false;
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
		StringStream ss(jsonBuf);

		rapidjson::Reader reader;
		MyHandler jsonHandler;
		TreeParser parser(builder);
		jsonHandler.stack.push(&parser);
		reader.Parse(ss, jsonHandler);

		builder->prepareDataMap();

		repoInfo << "Pre-processing scene bounds (BIM004 and below)...";

		// Get offset

		// In BIM004 and below, the bounds are not set, meaning we need to get
		// them from the geometry directly. We do this by reading the data section
		// once then resetting the stream.

		// For BIM005, we should introduce instancing, and along with that, the
		// primary offset applied at the root node transform.

		auto bounds = builder->readBoundsFromData(inbuf);
		builder->offset = -bounds.min();

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
		scene->setWorldOffset(builder->rootTransform * builder->offset);

		scene->setRevision(settings.getRevisionId());
		scene->setOriginalFiles({ filePath });
		scene->loadRootNode(handler.get());
		if (builder->hasMissingTextures()) {
			scene->setMissingTexture();
		}

		if (scene->isMissingTexture()){
			err = REPOERR_LOAD_SCENE_MISSING_TEXTURE;
		}

		return true;
	}
	else {
		repoError << "File " << fileName << " not found.";
		err = REPOERR_MODEL_FILE_READ;
		return false;
	}
}

repo::core::model::RepoScene* RepoModelImport::generateRepoScene(uint8_t& errCode)
{
	return scene;
}