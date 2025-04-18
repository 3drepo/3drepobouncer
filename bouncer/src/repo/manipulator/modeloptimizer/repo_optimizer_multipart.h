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

#include "../../core/model/collection/repo_scene.h"
#include "../../core/model/bson/repo_node_mesh.h"
#include "../../core/model/bson/repo_node_supermesh.h"
#include "../../core/model/bson/repo_node_material.h"

namespace repo {
	namespace manipulator {
		namespace modeloptimizer {
			class MultipartOptimizer
			{
			public:
				/**
				* Apply optimisation on the given repoScene
				* @param scene takes in a repoScene to optimise
				* @return returns true upon success
				*/
				bool apply(repo::core::model::RepoScene *scene);

			private:
				/*
				* Maps between two Repo UUIDs
				*/
				using UUIDMap = std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>;

				/*
				* A dictionary of MaterialNodes indexed by UUIDs
				*/
				using MaterialMap = std::unordered_map<repo::lib::RepoUUID, repo::core::model::MaterialNode*, repo::lib::RepoUUIDHasher>;

				/*
				* A dictionary of MeshNodes indexed by UUIDs. Each UUID may map to multiple nodes.
				*/
				using MeshMap = std::unordered_multimap<repo::lib::RepoUUID, repo::core::model::MeshNode, repo::lib::RepoUUIDHasher>;

				/**
				* Recursively enumerates scene to find all the instances of
				* MeshNodes that match the Id, and returns copies of them
				* with geometry baked to its world space location in the
				* scene graph.
				*/
				bool getBakedMeshNodes(
					const repo::core::model::RepoScene* scene,
					const repo::core::model::RepoNode* node,
					repo::lib::RepoMatrix mat,
					MeshMap &nodes);

				/**
				* Represents a batched set of geometry.
				*/
				struct mapped_mesh_t {
					std::vector<repo::lib::RepoVector3D> vertices;
					std::vector<repo::lib::RepoVector3D> normals;
					std::vector<repo::lib::repo_face_t> faces;
					std::vector<std::vector<repo::lib::RepoVector2D>> uvChannels;
					std::vector<repo::lib::repo_mesh_mapping_t> meshMapping;
				};

				void appendMesh(
					const repo::core::model::RepoScene* scene,
					repo::core::model::MeshNode node,
					mapped_mesh_t& mappedMesh
				);

				/*
				* Splits a MeshNode into a set of mapped_mesh_ts based on face location, so
				* each mapped_mesh_t has a vertex count below a certain size.
				*/
				void splitMesh(
					const repo::core::model::RepoScene* scene,
					repo::core::model::MeshNode node,
					std::vector<mapped_mesh_t> &mappedMeshes
				);

				/*
				* Turns a mapped_mesh_t into a MeshNode that can be added to the database
				*/
				repo::core::model::SupermeshNode* createSupermeshNode(
					const mapped_mesh_t& mapped,
					bool isGrouped
				);

				/**
				* Builds a set of Supermesh MeshNodes, based on the UUIDs in meshGroup.
				* The method may output an arbitrary number (including 1) supermeshes,
				* from an arbitrary number (including 1) UUIDs. If no UUIDs are provided,
				* no Supermeshes are created.
				* Each Supermesh mapping re-maps its Material UUID to a new UUID, which
				* is used to copy the materials. If an existing remapping exists, it is
				* used, otherwise one is created on demand.
				* Each Supermesh is guaranteed to be below a certain size.
				*/
				void createSuperMeshes(
					const repo::core::model::RepoScene *scene,
					const std::vector<repo::core::model::MeshNode> &nodes,
					const bool isGrouped,
					std::vector<repo::core::model::SupermeshNode*> &supermeshNodes);

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
				repo::lib::RepoUUID getMaterialID(
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
					const repo::core::model::MeshNode *mesh,
					repo::lib::RepoUUID &texID);

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
				* Creates a set of supermesh MeshNodes which contain the submeshes
				* in meshes (passed by UUID), along with a set of duplicate material
				* nodes for use by the supermeshes.
				* Multiple supermeshes may be created, depending on the composition
				* of meshes.
				* Supermeshes and duplicated materials are passed back via the
				* mergedMeshes and matNodes arrays.
				* The matIDs map is used to map between the original graph materials
				* and the stash graph materials, in case this method is called multiple
				* times for one graph.
				*/
				bool processMeshGroup(
					const repo::core::model::RepoScene *scene,
					const MeshMap &bakedMeshNodes,
					const std::set<repo::lib::RepoUUID>	&groupMeshIds,
					const repo::lib::RepoUUID &rootID,
					repo::core::model::RepoNodeSet &mergedMeshes,
					const bool isGrouped);

				/**
				* Groups the MeshNodes into sets based on their location and
				* geometry size.
				*/
				std::vector<std::vector<repo::core::model::MeshNode>> clusterMeshNodes(
					const std::vector<repo::core::model::MeshNode>& nodes
				);

				/**
				* Sort the given RepoNodeSet of meshes for multipart merging
				* @param scene             scene as reference
				* @param meshes            meshes to sort
				* @param normalMeshes      container to store normal meshes
				* @param transparentMeshes container to store (semi)transparent meshes
				* @param texturedMeshes    container to store textured meshes
				*/
				void sortMeshes(
					const repo::core::model::RepoScene *scene,
					const repo::core::model::RepoNodeSet &meshes,
					std::unordered_map<std::string, std::unordered_map<uint32_t, std::vector<std::set<repo::lib::RepoUUID>>>>	&normalMeshes,
					std::unordered_map < std::string, std::unordered_map<uint32_t, std::vector<std::set<repo::lib::RepoUUID>>>>	&transparentMeshes,
					std::unordered_map < std::string, std::unordered_map < uint32_t, std::unordered_map < repo::lib::RepoUUID,
					std::vector<std::set<repo::lib::RepoUUID>>, repo::lib::RepoUUIDHasher >>> &texturedMeshes
				);
			};
		}
	}
}
