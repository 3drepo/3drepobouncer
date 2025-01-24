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
#include "repo_material_builder.h"

#include <fstream>
#include <vector>
#include <string>

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				class GeometryCollector
				{
				public:
					GeometryCollector(repo::manipulator::modelutility::RepoSceneBuilder* builder);

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
					* Sets the current Layer (position in the tree) by the id used to create
					* the layer. This will be used for any following meshes.
					*/
					void setLayer(std::string id);

					/*
					* Adds a metadata node for the given layer. Each call will result in a new
					* node (so, if called twice a given transformation could end up with two
					* metadata nodes parented to it). If this is not desired, call hasMetadata
					* to see if the id already has had a node created.
					*/
					void setMetadata(std::string id, std::unordered_map<std::string, repo::lib::RepoVariant>);

					bool hasMetadata(std::string id);

					/*
					* Immediately changes which material any new geometry should be using.
					*/
					void setMaterial(const repo_material_t& material, bool missingTextures);

					void addFace(const std::vector<repo::lib::RepoVector3D64>& vertices) {
						meshBuilder->addFace(vertices);
					}
					void addFace(
						const std::vector<repo::lib::RepoVector3D64>& vertices,
						const repo::lib::RepoVector3D64& normal,
						const std::vector<repo::lib::RepoVector2D>& uvCoords) {
						meshBuilder->addFace(vertices, normal, uvCoords);
					}

					void setUnits(repo::manipulator::modelconvertor::ModelUnits units) {
						sceneBuilder->setUnits(units);
					}

					void setWorldOffset(repo::lib::RepoVector3D64 offset){
						sceneBuilder->setWorldOffset(offset);
					}

					/*
					* Should be called when the rendering is complete, to emit any remaining
					* meshes and materials.
					*/
					void finalise();

					/*
					* Sets the offset that should be applied to the geometry before it is
					* written to a MeshNode. If geometry is output in Project Coordinates,
					* then this should the inverse of the scene's World Offset. If it is
					* already in model space, then it should be zero (and the world location
					* in model space should be applied to the scene).
					*/
					void setGeometryOffset(repo::lib::RepoVector3D64 offset);

				private:

					repo::manipulator::modelutility::RepoSceneBuilder* sceneBuilder;

					repo_material_t GetDefaultMaterial() const;

					/*
					* This object is used as a deleter for the meshBuilder smart pointer. Using
					* a custom deleter ensures that we cannot accidentally abandon any
					* uncollected geometry.
					*/
					struct RepoMeshBuilderDeleter {
						RepoMeshBuilderDeleter(GeometryCollector* processor) :
							processor(processor)
						{}
						GeometryCollector* processor;
						void operator()(RepoMeshBuilder* builder);
					};

					using RepoMeshBuilderPtr = std::unique_ptr<RepoMeshBuilder, RepoMeshBuilderDeleter&>;

					RepoMeshBuilderDeleter meshBuilderDeleter; 	// (We only need one of these...)
					RepoMeshBuilderPtr meshBuilder;

					// Convenience method for building a new RepoMeshBuilder smart pointer;
					// there are no side effects so the std constructor could be used instead.
					RepoMeshBuilderPtr makeMeshBuilder(
						std::vector<repo::lib::RepoUUID> parents,
						repo::lib::RepoVector3D64 offset,
						repo_material_t material);

					repo::lib::RepoVector3D64 offset;
					RepoMaterialBuilder materialBuilder;

					std::unordered_map<std::string, repo::lib::RepoUUID> layerIdToSharedId;
					std::set<std::string> layersWithMetadata;

					repo::lib::RepoUUID rootNodeId;
				};
			}
		}
	}
}