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

#include "../../../../core/model/bson/repo_bson_factory.h"
#include "../../../../lib/datastructure/repo_structs.h"

#include <fstream>
#include <vector>
#include <string>


namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace odaHelper {
				struct mesh_data_t {
					std::vector<repo::lib::RepoVector3D64> rawVertices;
					std::vector<repo_face_t> faces;
					std::vector<std::vector<float>> boundingBox;
					std::unordered_map<unsigned long, int> vToVIndex;
					std::vector<repo::lib::RepoVector2D> uvCoords;
					std::string name;
					std::string layerName;
					std::string groupName;
					uint32_t matIdx;
				};

				class GeometryCollector
				{
				public:
					GeometryCollector();
					~GeometryCollector();

					/**
					* Get all the texture nodes collected.
					* @return returns a repoNodeSet containing texture nodes
					*/
					repo::core::model::RepoNodeSet getTextureNodes();

					/**
					* Check whether collector has missing textures.
					* @return returns true if at least one texture is missing
					*/
					bool hasMissingTextures();

					/**
					* Get all the material nodes collected.
					* @return returns a repoNodeSet containing material nodes
					*/
					repo::core::model::RepoNodeSet getMaterialNodes();


					/**
					* Get all the transformation nodes collected.
					* This is based on the layer information
					* @return returns a repoNodeSet containing transformation nodes
					*/
					repo::core::model::RepoNodeSet getTransformationNodes() {
						return transNodes;
					}

					/**
					* Get all mesh nodes collected.
					* @return returns a vector of mesh nodes
					*/
					repo::core::model::RepoNodeSet getMeshNodes();

					/**
					* Gt the model offset applied on the meshes collected
					* this is typically the minimum bounding box.
					* @return returns a vector of double (3 values) for the model offset.
					*/
					std::vector<double> getModelOffset() const {
						return minMeshBox;
					}

					/**
					* Indicates a start of a mesh
					*/
					void startMeshEntry();

					/**
					* Indicates end of a mesh
					*/
					void stopMeshEntry();

					void setLayer(const std::string &name) {
						nextLayer = name;
					}

					/** 
					* Set the name for the next mesh
					* @param name name of the next mesh
					*/
					void setNextMeshName(const std::string &name) {
						nextMeshName = name;
					}

					/**
					* Set next group name
					* @param groupName group name of the next mesh
					*/
					void setMeshGroup(const std::string &groupName) {
						nextGroupName = groupName;
					}

					/**
					* Add a face to the current mesh
					* @param vertices a vector of vertices that makes up this face
					*/
					void addFace(
						const std::vector<repo::lib::RepoVector3D64> &vertices,
						const std::vector<repo::lib::RepoVector2D>& uvCoords = std::vector<repo::lib::RepoVector2D>()
					);

					/**
					* Change current material to the one provided
					* @param material material contents.
					*/
					void setCurrentMaterial(const repo_material_t &material);


				private:
					std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<int, mesh_data_t>>> meshData;
					std::string nextMeshName, nextLayer, nextGroupName;
					std::unordered_map< uint32_t, repo::core::model::MaterialNode > idxToMat;					
					std::unordered_map<uint32_t, std::vector<repo::lib::RepoUUID> > matToMeshes;
					repo::core::model::RepoNodeSet transNodes;
					uint32_t currMat;
					std::vector<double> minMeshBox;
					mesh_data_t *currentEntry = nullptr;
					bool missingTextures = false;

					repo::core::model::TransformationNode* createTransNode(
						const std::string &name,
						const repo::lib::RepoUUID &parentId
					);

					mesh_data_t createMeshEntry();
				};
			}
		}
	}
}

