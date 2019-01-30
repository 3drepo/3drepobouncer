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
					* Get all the metadata nodes collected.
					* This is based on the layer information
					* @return returns a repoNodeSet containing metadata nodes
					*/
					repo::core::model::RepoNodeSet getMetadataNodes() {
						return metaNodes;
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
					* returns a boolean indicating if we already have metedata for this given ID
					* @return returns true if there is a metadata entry
					*/
					bool hasMeta(const std::string &id) {
						return idToMeta.find(id) != idToMeta.end();
					}

					/**
					* Indicates a start of a mesh
					*/
					void startMeshEntry();

					/**
					* Indicates end of a mesh
					*/
					void stopMeshEntry();

					void setLayer(const std::string id, const std::string &name) {
						nextLayer = id;
						if (layerIDToName.find(id) == layerIDToName.end())
							layerIDToName[id] = name;
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
					* @return returns true if there's already a metadata entry for this grouping
					*/
					void setMeshGroup(const std::string &groupName) {
						nextGroupName = groupName;
					}

					/**
					* Add a face to the current mesh
					* @param vertices a vector of vertices that makes up this face
					*/
					void addFace(
						const std::vector<repo::lib::RepoVector3D64> &vertices
					);

					/**
					* Change current material to the one provided
					* @param material material contents.
					*/
					void setCurrentMaterial(const repo_material_t &material);

					/**
					* Set metadata of a group
					* @param groupName groupName
					* @param metaEntry Metadata entry for groupName
					*/
					void setMetadata(const std::string &groupName,
						const std::unordered_map<std::string, std::string> &metaEntry)
					{
						idToMeta[groupName] = metaEntry;
					}


				private:

					std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<int, mesh_data_t>>> meshData;
					std::unordered_map<std::string, std::unordered_map<std::string, std::string> > idToMeta;
					std::unordered_map<std::string, std::string> layerIDToName;
					std::string nextMeshName, nextLayer, nextGroupName;
					std::unordered_map< uint32_t, repo::core::model::MaterialNode > idxToMat;					
					std::unordered_map<uint32_t, std::vector<repo::lib::RepoUUID> > matToMeshes;					
					repo::core::model::RepoNodeSet transNodes, metaNodes;
					uint32_t currMat;
					std::vector<double> minMeshBox;
					mesh_data_t *currentEntry = nullptr;

					repo::core::model::TransformationNode* createTransNode(
						const std::string &name,
						const std::string &id,
						const repo::lib::RepoUUID &parentId
					);


					repo::core::model::MetadataNode*  GeometryCollector::createMetaNode(
						const std::string &name,
						const repo::lib::RepoUUID &parentId,
						const  std::unordered_map<std::string, std::string> &metaValues
					);


					mesh_data_t createMeshEntry();
				};
			}
		}
	}
}

