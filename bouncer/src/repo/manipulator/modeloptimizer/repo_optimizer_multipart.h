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
#include "../../core/handler/database/repo_query.h"
#include "../../core/handler/fileservice/repo_blob_files_handler.h"
#include "../../lib/datastructure/repo_structs.h"
#include "bvh/bvh.hpp"
#include "bvh/sweep_sah_builder.hpp"
#include <repo/manipulator/modelconvertor/export/repo_model_export_abstract.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_node_streaming_mesh.h>

namespace repo {
	namespace manipulator {
		namespace modeloptimizer {

			// The vertex count is used as a rough approximation of the total geometry size.
			// This figure is empirically set to end up with an average bundle size of 24 Mb.
			#define REPO_MP_MAX_VERTEX_COUNT 1200000

			class MultipartOptimizer
			{
				typedef float Scalar;
				typedef bvh::Bvh<Scalar> Bvh;
				typedef bvh::Vector3<Scalar> BvhVector3;

				repo::core::handler::AbstractDatabaseHandler* handler;
				repo::manipulator::modelconvertor::AbstractModelExport* exporter;

			public:
				MultipartOptimizer(
					repo::core::handler::AbstractDatabaseHandler* handler,
					repo::manipulator::modelconvertor::AbstractModelExport* exporter
				);

				void processScene(
					std::string database,
					std::string collection,
					repo::lib::RepoUUID revId
				);

				bool splitByFloor;

			private:

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

				struct ProcessingJob {
					std::string description;
					repo::core::handler::database::query::RepoQuery filter;
					repo::lib::RepoUUID texId;

					bool isTexturedJob() const {
						return !texId.isDefaultValue();
					}
				};

				struct BranchGroup {
					std::string name; // The tag that gets associated with any supermeshes
				};

				/*
				* Contains the baked transformation information, i.e. the final conditions
				* of the Transformation just above a MeshNode.
				*/
				struct TransformInfo {
					repo::lib::RepoMatrix matrix;
					BranchGroup* branch = nullptr;
				};

				typedef std::unordered_map <repo::lib::RepoUUID, std::shared_ptr<repo::core::model::MaterialNode>, repo::lib::RepoUUIDHasher> MaterialPropMap;
				typedef std::unordered_map<repo::lib::RepoUUID, TransformInfo, repo::lib::RepoUUIDHasher> TransformMap;

				std::vector<BranchGroup> branchGroups;

				TransformMap getAllTransforms(
					const std::string &database,
					const std::string &collection,
					const repo::lib::RepoUUID &revId
				);

				void traverseTransformTree(
					const repo::core::model::RepoBSON &root,
					const std::unordered_map<repo::lib::RepoUUID, std::vector<repo::core::model::RepoBSON>, repo::lib::RepoUUIDHasher> &childNodeMap,
					TransformMap &transforms);

				/*
				* Uses metadata queries to find branching groups, and apply them to the
				* transform map. Currently the metadata queries are static.
				*/
				void applyBranchGroups(
					TransformMap& transforms, 
					const std::string& database,
					const std::string& collection, 
					const repo::lib::RepoUUID &revId);

				MaterialPropMap getAllMaterials(
					const std::string &database,
					const std::string &collection,
					const repo::lib::RepoUUID &revId
				);

				std::vector < repo::lib::RepoUUID> getAllTextureIds(
					const std::string &database,
					const std::string &collection,
					const repo::lib::RepoUUID &revId
				);

				ProcessingJob createUntexturedJob(
					const repo::lib::RepoUUID &revId,
					const int primitive,
					const bool isOpaque,
					const bool hasNormals
				);

				ProcessingJob createTexturedJob(
					const repo::lib::RepoUUID &revId,
					const int primitive,
					const bool hasNormals,
					const repo::lib::RepoUUID &texId
				);

				void clusterAndSupermesh(
					const std::string &database,
					const std::string &collection,
					const TransformMap& transformMap,
					const MaterialPropMap& matPropMap,
					const ProcessingJob &job
				);

				void createSuperMeshes(
					const std::string &database,
					const std::string &collection,
					const TransformMap& transformMap,
					const MaterialPropMap& matPropMap,
					std::vector<repo::core::model::StreamingMeshNode>& meshNodes,
					const std::vector<std::vector<int>>& clusters,
					const repo::lib::RepoUUID &texId,
					const std::string& namedGrouping
				);

				void createSuperMesh(
					const mapped_mesh_t& mappedMesh,
					const std::string& tag
				);

				void appendMesh(					
					repo::core::model::StreamingMeshNode &node,
					const MaterialPropMap &matPropMap,
					mapped_mesh_t &mappedMesh,
					const repo::lib::RepoUUID &texId
				);

				Bvh buildFacesBvh(
					repo::core::model::StreamingMeshNode &node
				);

				void flattenBvh(
					const Bvh &bvh,
					std::vector<size_t> &leaves,
					std::vector<size_t> &branches
				);

				std::vector<size_t> getBranchPrimitives(
					const Bvh& bvh,
					size_t head
				);

				/*
				* Turns a mapped_mesh_t into a MeshNode that can be added to the database
				*/
				std::unique_ptr < repo::core::model::SupermeshNode> createSupermeshNode(
					const mapped_mesh_t &mapped
				);

				/*
				* Splits a MeshNode into a set of mapped_mesh_ts based on face location, so
				* each mapped_mesh_t has a vertex count below a certain size.
				*
				* To split the mesh in a memory efficient way, we use an advancing front approach together
				* with some efficient use of pointer.
				*
				* First, a BVH for the faces is build. Then the tree is flattened so that we have the leaves
				* and all branch nodes separately. The branch nodes will be in top-down order.

				* The front is represented as two vectors of unique pointers. Both have the same length as the
				* number of nodes in the bvh and are linked to them by their index (i.e. field i in the vector
				* belongs to node i in the tree). The unique pointers are typed for sets for the vertex indices
				* and for vectors for the primitive indices (the faces).
				* They are both initially holding only nullptrs and will only ever hold valid pointers for the
				* nodes currently relevant to the front. This allows to only hold the data that is really needed
				* in memory at any time.
				*
				* First, the leaves are processed. For each, the unique vertex indices and the face indices are
				* collected, the sets and vectors created, and the pointers to these attached to the front.
				*
				* Then, the branch nodes are traversed in reverse order. This order ensures that for each node
				* that the processing reaches, the two children have been processed previously.
				* At reaching a branch node, the state of their children is checked, their unique vertex indexes
				* merged, and their count checked against the threshold.
				* If the threshold is passed, the children are "cut off" i.e. processed to super meshes and written
				* out.
				* If the threshold is not passed, the new set and vector are attached to the current node and the
				* sets/vectors of the children are deleted and the memory associated with it released.
				* Nodes that have only one child are handled by moving the pointers to the parent without a copy.
				*
				* At reaching the end, it is possible to have vertices "left over" that come from branches that
				* never exceeded the threshold. They are then gathered in one last super mesh.
				*/
				void splitMesh(
					repo::core::model::StreamingMeshNode& node,
					const MaterialPropMap& matPropMap,
					const repo::lib::RepoUUID& texId,
					const std::string& namedGrouping
				);

				void createSupermeshFromBranch(
					repo::core::model::StreamingMeshNode& node,
					const MaterialPropMap& matPropMap,
					const repo::lib::RepoUUID& texId,
					std::set<uint32_t>* globalVertexIndices,
					std::vector<uint32_t>* primitives,
					const std::string& namedGrouping
				);

				/**
				* Groups the MeshNodes into sets based on their location and
				* geometry size.
				*/
				std::vector<std::vector<int>> clusterMeshNodes(
					const std::vector<repo::core::model::StreamingMeshNode>& nodes
				);

				void clusterMeshNodesBvh(
					const std::vector<repo::core::model::StreamingMeshNode>& meshes,
					const std::vector<int>& binIndexes,
					std::vector<std::vector<int>>& clusters);

				Bvh buildBoundsBvh(
					const std::vector<int>& binIndexes,
					const std::vector<repo::core::model::StreamingMeshNode>& meshes
				);

				std::vector<size_t> getVertexCounts(
					const Bvh& bvh,
					const std::vector<int>& binIndexes,
					const std::vector<repo::core::model::StreamingMeshNode>& meshes
				);

				std::vector<size_t> getSupermeshBranchNodes(
					const Bvh &bvh,
					const std::vector<size_t> &vertexCounts);

				void splitBigClusters(
					std::vector<std::vector<int>>& clusters
				);
								
			};
		}
	}
}
