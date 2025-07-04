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
			class MultipartOptimizer
			{
				
				typedef float Scalar;
				typedef bvh::Bvh<Scalar> Bvh;
				typedef bvh::Vector3<Scalar> BvhVector3;

			public:
				
				bool processScene(
					std::string database,
					std::string collection,
					repo::lib::RepoUUID revId,
					repo::core::handler::AbstractDatabaseHandler *handler,
					repo::manipulator::modelconvertor::AbstractModelExport *exporter
				);


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
				

				typedef std::unordered_map <repo::lib::RepoUUID, std::shared_ptr<repo::core::model::MaterialNode>, repo::lib::RepoUUIDHasher> MaterialPropMap;
				typedef std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> TransformMap;

				std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> getAllTransforms(
					repo::core::handler::AbstractDatabaseHandler *handler,
					const std::string &database,
					const std::string &collection,
					const repo::lib::RepoUUID &revId
				);

				void traverseTransformTree(
					const repo::core::model::RepoBSON &root,
					const std::unordered_map<repo::lib::RepoUUID, std::vector<repo::core::model::RepoBSON>, repo::lib::RepoUUIDHasher> &childNodeMap,
					std::unordered_map<repo::lib::RepoUUID,	repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> &transforms);

				MaterialPropMap getAllMaterials(
					repo::core::handler::AbstractDatabaseHandler *handler,
					const std::string &database,
					const std::string &collection,
					const repo::lib::RepoUUID &revId
				);

				std::set<std::string> getAllGroupings(
					repo::core::handler::AbstractDatabaseHandler* handler,
					const std::string& database,
					const std::string& collection,
					const repo::lib::RepoUUID& revId
				);

				std::vector < repo::lib::RepoUUID> getAllTextureIds(
					repo::core::handler::AbstractDatabaseHandler *handler,
					const std::string &database,
					const std::string &collection,
					const repo::lib::RepoUUID &revId,
					const std::string& grouping
				);

				ProcessingJob createUntexturedJob(
					const std::string &description,
					const repo::lib::RepoUUID &revId,
					const int primitive,
					const std::string &grouping,
					const bool isOpaque
				);

				ProcessingJob createTexturedJob(
					const std::string &description,
					const repo::lib::RepoUUID &revId,
					const int primitive,
					const std::string &grouping,
					const repo::lib::RepoUUID &texId
				);

				void clusterAndSupermesh(
					const std::string &database,
					const std::string &collection,
					repo::core::handler::AbstractDatabaseHandler *handler,
					repo::manipulator::modelconvertor::AbstractModelExport *exporter,
					const TransformMap& transformMap,
					const MaterialPropMap& matPropMap,
					const ProcessingJob &job
				);

				void createSuperMeshes(
					const std::string &database,
					const std::string &collection,
					repo::core::handler::AbstractDatabaseHandler *handler,
					repo::manipulator::modelconvertor::AbstractModelExport *exporter,
					const TransformMap& transformMap,
					const MaterialPropMap& matPropMap,
					std::vector<repo::core::model::StreamingMeshNode>& meshNodes,
					const std::vector<std::vector<int>>& clusters,
					const repo::lib::RepoUUID &texId
				);

				void createSuperMesh(
					repo::manipulator::modelconvertor::AbstractModelExport *exporter,
					const mapped_mesh_t& mappedMesh
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

				std::vector<std::set<uint32_t>> getUniqueVertices(
					const Bvh& bvh,
					const std::vector<repo::lib::repo_face_t>& primitives // The primitives in this tree are faces
				);

				/*
				* Splits a MeshNode into a set of mapped_mesh_ts based on face location, so
				* each mapped_mesh_t has a vertex count below a certain size.
				*/
				void splitMesh(
					repo::core::model::StreamingMeshNode &node,
					repo::manipulator::modelconvertor::AbstractModelExport *exporter,
					const MaterialPropMap &matPropMap,
					const repo::lib::RepoUUID &texId
				);

				/*
				* Turns a mapped_mesh_t into a MeshNode that can be added to the database
				*/
				std::unique_ptr < repo::core::model::SupermeshNode> createSupermeshNode(
					const mapped_mesh_t &mapped
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
