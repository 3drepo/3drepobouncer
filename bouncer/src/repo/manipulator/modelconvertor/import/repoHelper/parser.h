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
#include "repo/manipulator/modelutility/repo_json_parser.h"

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
					repo::lib::RepoMatrix matrix;
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

				using namespace repo::manipulator::modelutility::json;

				struct MetadataParser : public Parser
				{
					std::unordered_map<std::string, repo::lib::RepoVariant>& metadata;
					repo::manipulator::modelutility::json::RepoVariantParser valueParser;

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
						TreeParser parser(builder);
						JsonParser::ParseJson(buf, &parser);
					}
				};
			}
		}
	}
}