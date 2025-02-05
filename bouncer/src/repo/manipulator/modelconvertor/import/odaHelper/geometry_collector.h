/**
*  Copyright (C) 2018 3D Repo Ltd
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

#include "repo/error_codes.h"
#include "repo/manipulator/modelconvertor/import/repo_model_units.h"
#include "repo/lib/datastructure/repo_structs.h"
#include "repo/manipulator/modelutility/repo_scene_builder.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/model/bson/repo_node_mesh.h"
#include "repo_mesh_builder.h"

#include <fstream>
#include <vector>
#include <string>
#include <stack>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class GeometryCollector
				{
				public:
					GeometryCollector(repo::manipulator::modelutility::RepoSceneBuilder* builder);
					~GeometryCollector();

					class Layer {
						std::string name;
						repo::lib::RepoUUID sharedId; // For setting up parents before the layer is actually committed.

						~Layer();
					};

					/*
					* Declares a new entry in the tree that can be accessed by an arbitrary
					* Id (instead of a RepoUUID). Layers must be explicitly set with a call
					* to setLayer. If parentId is empty, the layer will be created under the
					* root node.
					*/
					void createLayer(std::string id, std::string name, std::string parentId);

					/*
					* True if the layer with the id was creaed with createLayer. Use this to
					* check if a layer exists before trying to create or set it.
					*/
					bool hasLayer(std::string id);

					/*
					* Returns the sharedId for a layer with the given local id; used for
					* setting the parents of mesh and metadata nodes.
					*/
					repo::lib::RepoUUID getSharedId(std::string id);

					/*
					* Adds a metadata node for the given layer. Each call will result in a new
					* node (so, if called twice a given transformation could end up with two
					* metadata nodes parented to it). If this is not desired, call hasMetadata
					* to see if the id already has had a node created.
					*/
					void setMetadata(std::string id, std::unordered_map<std::string, repo::lib::RepoVariant>);

					bool hasMetadata(std::string id);

					void addMeshes(std::string id, std::vector<std::pair<repo::core::model::MeshNode, repo::lib::repo_material_t>>& meshes);

					/*
					* Immediately changes which material any new geometry should be using.
					*/
					void setMaterial(const repo::lib::repo_material_t& material);

					/*
					* A stack allocated triangle that can have uvs and a normal. This takes only
					* a subset of formats supported by the face.
					*/
					struct Face
					{
						Face() :
							numUvs(0),
							numVertices(0)
						{
						}

						repo::lib::RepoVector3D64 vertices[3];
						repo::lib::RepoVector2D uvs[3];
						int numUvs;
						int numVertices;

						void push_back(repo::lib::RepoVector3D64 vertex) {
							vertices[numVertices++] = vertex;
						}

						void push_back(repo::lib::RepoVector2D uv) {
							uvs[numUvs++] = uv;
						}
					};

					void addFace(const std::initializer_list<repo::lib::RepoVector3D64>& vertices);

					void addFace(const Face& triangle);

					template<repo::core::model::RepoNodeClass T>
					void addNode(const T& n) {
						sceneBuilder->addNode(n);
					}

					void addMaterialReference(repo::lib::repo_material_t material, repo::lib::RepoUUID parentId) {
						sceneBuilder->addMaterialReference(material, parentId);
					}

					void setUnits(repo::manipulator::modelconvertor::ModelUnits units) {
						sceneBuilder->setUnits(units);
					}

					void setWorldOffset(repo::lib::RepoVector3D64 offset){
						sceneBuilder->setWorldOffset(offset);
					}

					repo::lib::RepoVector3D64 getWorldOffset() {
						return sceneBuilder->getWorldOffset();
					}

					/*
					* Should be called when the rendering is complete, to emit any remaining
					* meshes and materials.
					*/
					void finalise();

					/*
					* Maintains a set of MeshBuilders for given materials intended to receive
					* geometry under a single transformation, until the active context changes.
					*/
					class Context
					{
					public:
						Context(repo::lib::RepoVector3D64 offset, const repo::lib::repo_material_t& material):
							offset(offset)
						{
							setMaterial(material);
						}

						~Context();

						std::unordered_map<uint64_t, std::unique_ptr<RepoMeshBuilder>> meshBuilders;

						/*
						* Creates a MeshBuilder for the material, and sets it as the active
						* one.
						*/
						void setMaterial(const repo::lib::repo_material_t& material);

						void addFace(const std::initializer_list<repo::lib::RepoVector3D64>& vertices) {
							meshBuilder->addFace(RepoMeshBuilder::face(
								vertices.begin(), vertices.size()
							));
						}

						void addFace(const Face& triangle) {
							meshBuilder->addFace(RepoMeshBuilder::face(
								triangle.vertices, triangle.numVertices,
								triangle.uvs, triangle.numUvs
							));
						}

						std::vector<std::pair<repo::core::model::MeshNode, repo::lib::repo_material_t>> extractMeshes();

					private:
						repo::lib::RepoVector3D64 offset;
						RepoMeshBuilder* meshBuilder;
					};

					/*
					* Creates a context initialised with the offset and material. This returns
					* a value, with the expectation that it will be used inside a subclass
					* constructor.
					*/
					Context createDrawContext();

					// These stack operations must be symmetric - calling pop with a different
					// pointer to the active context will result in an exception.

					/*
					* If ctx is not null, sets the active context to ctx, and stores the
					* previous context on the stack.
					*/
					void pushDrawContext(Context* ctx);

					/*
					* If ctx is not null, sets the active context to the previous one.
					*/
					void popDrawContext(Context* ctx);

					repo::lib::repo_material_t getLastMaterial();

				private:
					repo::manipulator::modelutility::RepoSceneBuilder* sceneBuilder;
					std::stack<Context*> contexts;
					repo::lib::repo_material_t latestMaterial;
					std::unordered_map<std::string, repo::lib::RepoUUID> layerIdToSharedId;
					std::set<std::string> layersWithMetadata;
					repo::lib::RepoUUID rootNodeId;
				};
			}
		}
	}
}