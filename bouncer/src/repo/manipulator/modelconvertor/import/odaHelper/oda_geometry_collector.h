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
					const std::vector<repo::lib::RepoVector3D64> rawVertices;
					const std::vector<repo_face_t> faces;
					const std::vector<std::vector<float>> boundingBox;
					const std::string name;
					const uint32_t matIdx;
				};

				class OdaGeometryCollector
				{
				public:
					OdaGeometryCollector();
					~OdaGeometryCollector();

					std::vector<uint32_t> getMaterialMappings() const {
						return matVector;
					}

					repo::core::model::RepoNodeSet getMaterialNodes() {
						repo::core::model::RepoNodeSet matSet;
						for (const auto &matPair : idxToMat) {
							auto matIdx = matPair.first;
							if (matToMeshes.find(matIdx) != matToMeshes.end()) {
								matSet.insert(new repo::core::model::MaterialNode(matPair.second.cloneAndAddParent(matToMeshes[matIdx])));
							}
						}

						return matSet;
					}

					std::vector<repo::core::model::MeshNode> getMeshes();

					std::vector<double> getModelOffset() const {
						return minMeshBox;
					}

					void addMeshEntry(const std::vector<repo::lib::RepoVector3D64> &rawVertices,
						const std::vector<repo_face_t> &faces,
						const std::vector<std::vector<double>> &boundingBox
					);

					void addMaterialWithColor(const uint32_t &r, const uint32_t &g, const uint32_t &b, const uint32_t &a);

					void addMaterial(const uint64_t &matIndex, const repo_material_t &material);

					void setNextMeshName(const std::string &name) {
						nextMeshName = name;
					}

				private:
					std::vector<mesh_data_t> meshData;
					std::vector<uint32_t> matVector;
					std::unordered_map< uint32_t, repo::core::model::MaterialNode > idxToMat, codeToMat;					
					std::unordered_map<uint32_t, std::vector<repo::lib::RepoUUID> > matToMeshes;
					std::vector<double> minMeshBox;
					std::string nextMeshName;
					uint32_t currMat;
					std::ofstream ofile;
					int nVectors = 1;
				};
			}
		}
	}
}

