/**
*  Copyright (C) 2017 3D Repo Ltd
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

#include <repo/repo_controller.h>
namespace repo{
	namespace lib{
		class CSharpWrapper
		{
		public:

			~CSharpWrapper();

			/**
			* Singleton class - get its instance
			*/
			static CSharpWrapper* getInstance();
			
			static void destroyInstance();

			/**
			* Connect to a mongo database, authenticate by the admin database
			* @param config configuration file path
			* @return returns true if successfully connected, false otherwise
			*/
			bool connect(
				const std::string &config);


			/*
	 		 * Free memory occupied by the super mesh at a given index
 		 	 * @param meshIndex index of the super mesh to free
			 */
			void freeSuperMesh(const int &index);

			/**
			* Get ID Map buffer for submesh
			* @param index supermesh index
			* @param smIndex submesh index
			* @param idMap pre-allocated buffer for the ID Map
			*/
			void getIdMapBuffer(const int &index, const int &smIndex, float* &idMap) const;

			/**
			* Retrieve the information of the materials given the material ID
			* @param materialID ID of the material
			* @param diffuse (output) pre-allocated array of 4 floats for diffuse colour
			* @param diffuse (output) pre-allocated array of 4 floats for specular colour
			* @param shiniess (output) shininess value
			* @param shiniess strength (output) shininess strength value
			*/
			void getMaterialInfo(
				const std::string &materialID,
				float* &diffuse,
				float* &specular,
				float* &shininess,
				float* &shininessStrength) const;

			/**
			* Get original mesh information from submesh
			* @param index supermesh Index
			* @param smIndex sub mesh index
			* @param orgMesh index within the sub mesh
			* @param smFfrom (output) starting index of faces
			* @param smFto   (output) ending index of faces
			* @return return material ID
			*/
			std::string getOrgMeshInfoFromSubMesh(
				const int &index,
				const int &smIndex,
				const int &orgMeshIdx,
				int* &smFFrom,
				int* &smFTo) const ;

			/**
			* Get number of super meshes in the loaded project
			* @return returns the number of super meshes in this project
			*/
			int getNumSuperMeshes() const;

			/**
			 * Retrieve an array of revision ID
			 * @param database name of the database
			 * @param project name of the project
			 * @return returns array to store the array of revisions to
			 */
			std::string getRevisionList(
				const std::string &database,
				const std::string &project);


			/**
			* Retrieve location of buffers for this supermesh submesh
			* @param index supermesh index
			* @param subIdx submesh index
			* @param vFrom  (output) start index of vertices
			* @param vTo  (output) end index of vertices
			* @param fFrom  (output) start index of faces
			* @param fTo  (output) end index of faces
			*/
			void getSubMeshInfo(
				const int &index,
				const int &subIdx,
				int32_t* &vFrom,
				int32_t* &vTo,
				int32_t* &fFrom,
				int32_t* &fTo) const;

			/**
			* Copy over buffers from superMesh
			* @param index the index of the supermesh within the supermesh array
			* @param vertices pre-allocated vertice buffer
			* @param normals pre-allocated normal buffer
			* @param faces pre-allocated face buffer
			* @param idMap pre-allocated idMap buffer
			* @param numOrgMeshes number of org meshes per submesh
			*/
			void getSuperMeshBuffers(
				const int &index,
				float* &vertices,
				float* &normals,
				uint16_t* &faces,
				float* &uvs,
				int* &numOrgMeshes) const;

			/**
			* Get info on a supermesh
			* @param index the index of the supermesh within the supermesh array
			* @param subMeshes number of sub meshes within this mesh
			* @param numVertices (output) the number of vertices
			* @param numFaces (output) number of faces
			* @param hasNormals (output) return true if it has Normals
			* @param hasUV (output) return true if it has UVs
			* @return return the unique ID of the supermesh as a string
			*/
			std::string getSuperMeshInfo(
				const int &index,
				long* &subMeshes,
				long* &numVertices,
				long* &numFaces,
				bool* &hasNormals,
				bool* &hasUV,
				int* &primitiveType,
				int* &numMappings) const;

			/**
			* Get texture binary for the given material
			* @param materialID Unique ID of the material as a string
			* @param texBuf preallocated buffer to copy texture buffer into
			*/
			void getTexture(
				char* materialID,
				char* texBuf) const;


			/**
			 * Get Wrapper versioning info
			 */
			std::string getVersion() const;

			/**
			* Check if the material has textures
			* @param materialID Unique ID of the material as a string
			* @return returns size of texture, 0 if no texture
			*/
			int hasTexture(
				const std::string &materialID) const;


			/**
			* Check if thegiven model is a federation of models
			* @param database name of the database
			* @param project name of the project
			* @return returns true if it is a federation, false otherwise
			*/
			bool isFederation(
				const std::string &database,
				const std::string &project) const;

			/**
			* Load a Repo scene from the database, in preparation for asset bundle generation
			* @param database database where the project is
			* @param project name of the project
			* @param revisionID revision ID
			* @return returns true if successfully loaded the scene, false otherwise
			*/
			bool initialiseSceneForAssetBundle(
				const std::string &database,
				const std::string &project,
				const std::string &revisionID);

			/**
			 * Check if VR is enabled for this model
			 * @return returns true if it is enabled, false otherwise
			 */
			bool isVREnabled();


			/**
			* Save asset bundles into the database
			* @param assetfiles array of file paths to assets
			* @param length length of the assetfiles array
			*/
			bool saveAssetBundles(
				char** assetFiles,
				int      length
				);

			static CSharpWrapper* wrapper;
		private:
			CSharpWrapper();
			repo::RepoController* controller;
			repo::RepoController::RepoToken* token;
			std::string projectName, databaseName;
			std::vector<std::shared_ptr<repo::core::model::MeshNode>> superMeshes;
			std::unordered_map<std::string, std::vector<uint8_t>> jsonFiles;
			repo::core::model::RepoUnityAssets unityAssets;
			std::vector<std::vector<uint16_t>> serialisedFaceBuf;
			std::vector<int> numIdMappings;
			std::vector<std::vector<std::vector<float>>> idMapBuf;
			std::vector<std::vector<std::vector<repo_mesh_mapping_t>>> meshMappings;
			std::unordered_map <std::string, repo_material_t> matNodes;
			std::unordered_map <std::string, std::vector<char>> matToTexture;
			repo::core::model::RepoScene *scene;
			bool vrEnabled = false;
		};
	}
}

