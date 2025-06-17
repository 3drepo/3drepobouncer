/**
*  Copyright (C) 2025 3D Repo Ltd
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

#pragma once

#include "repo/lib/datastructure/repo_structs.h"
#include "../rapidjson/reader.h"

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace repoHelper {

				/*
				* This header contains the types that when instantiated and
				* organised into a tree, form the event based parser that
				* outputs the records in the .bim header.
				*/

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

				class AbstractBuilder
				{
				public:
					virtual void createNode(const NodeRecord& r) = 0;
					virtual void createMaterial(repo::lib::repo_material_t material, int texture) = 0;
					virtual void createTexture(const TextureRecord& t) = 0;
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
					virtual Parser* Key(const std::string_view& key) { return nullptr; }
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

				struct MyHandler : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, MyHandler>
				{
					bool StartObject() {
						if (stack.top()) {
							stack.top()->StartObject();
						}
						stack.push(nullptr); // This one is to hold the Parsers for the objects' members. This should be updated after the call to Key before any values are read.
						return true;
					}

					bool Key(const Ch* str, rapidjson::SizeType length, bool copy) {
						stack.pop();
						stack.push(stack.top() ? stack.top()->Key(std::string_view(str, length)) : nullptr);
						return true;
					}

					bool EndObject(rapidjson::SizeType) {
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

					bool EndArray(rapidjson::SizeType) {
						stack.pop();
						if (stack.top()) {
							stack.top()->EndArray();
						}
						return true;
					}

					bool String(const Ch* str, rapidjson::SizeType length, bool copy) {
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

					BoundsParser(repo::lib::RepoBounds& bounds) :
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

					RangeParser(Range& range) :
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

					virtual void Number(float d) override {
						if (i < 3) {
							((float*)&colour)[i++] = d;
						}
					}
				};

				struct TextureParser : public ObjectParser
				{
					TextureRecord texture;
					AbstractBuilder* builder;

					TextureParser(AbstractBuilder* builder)
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
					AbstractBuilder* builder;

					MaterialParser(AbstractBuilder* builder)
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
					AbstractBuilder* builder;

					NodeParser(AbstractBuilder* builder)
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
					TreeParser(AbstractBuilder* builder)
					{
						parsers["nodes"] = new ArrayParser(new NodeParser(builder));
						parsers["materials"] = new ArrayParser(new MaterialParser(builder));
						parsers["textures"] = new ArrayParser(new TextureParser(builder));
					}

					static void ParseJson(const char* buf, AbstractBuilder* builder)
					{
						rapidjson::StringStream ss(buf);

						MyHandler jsonHandler;
						TreeParser parser(builder);
						jsonHandler.stack.push(&parser);

						rapidjson::Reader reader;
						reader.Parse(ss, jsonHandler);
					}
				};
			}
		}
	}
}