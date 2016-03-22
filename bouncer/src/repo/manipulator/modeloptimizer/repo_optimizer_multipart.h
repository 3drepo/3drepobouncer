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

/**
* Abstract optimizer class
*/

#pragma once

#include "repo_optimizer_abstract.h"
#include "../../core/model/collection/repo_scene.h"

#define REPO_MP_TEXTURE_WORK_AROUND

namespace repo {
	namespace manipulator {
		namespace modeloptimizer {
			class MultipartOptimizer : AbstractOptimizer
			{
			public:
				/**
				* Default constructor
				*/
				MultipartOptimizer();

				/**
				* Default deconstructor
				*/
				virtual ~MultipartOptimizer();


				/**
				* Apply optimisation on the given repoScene
				* @param scene takes in a repoScene to optimise
				* @return returns true upon success
				*/
				bool apply(repo::core::model::RepoScene *scene);

			private:
				/**
				* A Recursive call to traverse down the scene graph,
				* collect all the mesh data that is within meshGroup
				* @param scene scene to traverse
				* @param node current node
				* @param meshGroup the group of meshes to look for
				* @param matrix matrix to transform the vertices
				* @param vertices vertices collected
				* @param normals normals collected
				* @param faces faces collected
				* @param uvChannels uvChannels collected
				* @param colors colors collected
				* @param meshMapping meshMapping for this superMesh
				*/
#ifdef REPO_MP_TEXTURE_WORK_AROUND
				bool collectMeshData(
					const repo::core::model::RepoScene        *scene,
					const repo::core::model::RepoNode         *node,
					const std::set<repoUUID>                  &meshGroup,
					std::vector<float>                        &mat,
					std::vector<std::vector<repo_vector_t>>                &vertices,
					std::vector<std::vector<repo_vector_t>>               &normals,
					std::vector<std::vector<repo_face_t>>                &faces,
					std::vector<std::vector<std::vector<repo_vector2d_t>>> &uvChannels,
					std::vector<std::vector<repo_color4d_t>>               &colors,
					std::vector<std::vector<repo_mesh_mapping_t>>          &meshMapping
					);
#endif
				bool collectMeshData(
					const repo::core::model::RepoScene        *scene,
					const repo::core::model::RepoNode         *node,
					const std::set<repoUUID>                  &meshGroup,
					std::vector<float>                        &mat,
					std::vector<repo_vector_t>                &vertices,
					std::vector<repo_vector_t>                &normals,
					std::vector<repo_face_t>                  &faces,
					std::vector<std::vector<repo_vector2d_t>> &uvChannels,
					std::vector<repo_color4d_t>               &colors,
					std::vector<repo_mesh_mapping_t>          &meshMapping
					);

				/**
				* Merge all meshes within the mesh group and generate a
				* super mesh.
				* @param scene where the meshes are
				* @param meshGroup contains all the meshes to c merge
				* @param matIDs the unique IDs f materials required by this mesh
				* @return returns a pointer to a newly created merged mesh
				*/
#ifdef REPO_MP_TEXTURE_WORK_AROUND
				std::vector<repo::core::model::MeshNode*>createSuperMesh(
					const repo::core::model::RepoScene *scene,
					const std::set<repoUUID>           &meshGroup,
					std::set<repoUUID>                 &matIDs,
					const bool                         &texture);
#endif
				repo::core::model::MeshNode* createSuperMesh(
					const repo::core::model::RepoScene *scene,
					const std::set<repoUUID>           &meshGroup,
					std::set<repoUUID>                 &matIDs);

				/**
				* Generate the multipart scene 
				* @param scene scene to base on, this will also be modified to store the stash graph
				* @return returns true upon success
				*/
				bool generateMultipartScene(repo::core::model::RepoScene *scene);

				/**
				* Get child's material id from a mesh
				* @param scene scene it belongs to
				* @param mesh mesh in question
				* @return returns unqiue ID of the material
				*/
				repoUUID getMaterialID(
					const repo::core::model::RepoScene *scene,
					const repo::core::model::MeshNode  *mesh
					);

				/**
				* Check if the mesh has texture
				* @param scene scene as reference
				* @param mesh mesh to check
				* @param texID texID to return (if any)
				* @return returns true if there is texture
				*/
				bool hasTexture(
					const repo::core::model::RepoScene *scene,
					const repo::core::model::MeshNode  *mesh,
					repoUUID                           &texID);

				/**
				* Check if the mesh is (semi) Transparent
				* @param scene scene as reference
				* @param mesh mesh to check
				* @return returns true if the mesh is not fully opaque
				*/
				bool isTransparent(
					const repo::core::model::RepoScene *scene,
					const repo::core::model::MeshNode  *mesh); 

				/**
				* Process a mesh grouping, great a merged mesh base on the information
				* @param scene as reference
				* @param meshes mesh groupings
				* @param mergedMeshes add newly created meshes into this set
				* @param matNodes contains already processed materials
				*/
#ifdef REPO_MP_TEXTURE_WORK_AROUND
				bool processMeshGroup(
					const repo::core::model::RepoScene                                         *scene,
					const std::set<repoUUID>							                       & meshes,
					const repoUUID                                                             &rootID,
					repo::core::model::RepoNodeSet                                             &mergedMeshes,
					std::unordered_map<repoUUID, repo::core::model::RepoNode*, RepoUUIDHasher> &matNodes,
					const bool                                                                 &texture
					);
#endif
				bool processMeshGroup(
					const repo::core::model::RepoScene                                         *scene,
					const std::set<repoUUID>							                       & meshes,
					const repoUUID                                                             &rootID,
					repo::core::model::RepoNodeSet                                             &mergedMeshes,
					std::unordered_map<repoUUID, repo::core::model::RepoNode*, RepoUUIDHasher> &matNodes);

				/**
				* Sort the given RepoNodeSet of meshes for multipart merging
				* @param scene             scene as reference
				* @param meshes            meshes to sort
				* @param normalMeshes      container to store normal meshes
				* @param transparentMeshes container to store (semi)transparent meshes
				* @param texturedMeshes    container to store textured meshes
				*/
				void sortMeshes(
					const repo::core::model::RepoScene                                      *scene,
					const repo::core::model::RepoNodeSet                                    &meshes,
					std::unordered_map<uint32_t, std::vector<std::set<repoUUID>>>			&normalMeshes,
					std::unordered_map<uint32_t, std::vector<std::set<repoUUID>>>			&transparentMeshes,
					std::unordered_map<uint32_t, std::unordered_map<repoUUID,
										 std::vector<std::set<repoUUID>>, RepoUUIDHasher >> &texturedMeshes);

			};



		}
	}
}
