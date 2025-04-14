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

#include <submodules/asset_generator/src/repo_model_export_repobundle.h>

#include "bvh/bvh.hpp"
#include "bvh/sweep_sah_builder.hpp"



namespace repo {
	namespace manipulator {
		namespace modeloptimizer {
			class MultipartOptimizer
			{

				typedef float Scalar;
				typedef bvh::Bvh<Scalar> Bvh;
				typedef bvh::Vector3<Scalar> BvhVector3;				

				//using Scalar = float;
				//using Bvh = bvh::Bvh<Scalar>;
				//using BvhVector3 = bvh::Vector3<Scalar>;

			public:
				
				void ProcessScene(
					std::string database,
					std::string collection,
					repo::lib::RepoUUID revId,
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler
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

				// TODO: Is this in the right place here?
				class StreamingMeshNode {


					class SupermeshingData {

						repo::lib::RepoUUID uniqueId;

						// Geometry
						std::vector<repo::lib::RepoVector3D> vertices;
						std::vector<repo::lib::repo_face_t> faces;
						std::vector<repo::lib::RepoVector3D> normals;
						std::vector<std::vector<repo::lib::RepoVector2D>> channels;

					public:
						SupermeshingData(repo::core::model::RepoBSON& bson, std::vector<uint8_t>& buffer)
						{

							this->uniqueId = bson.getUUIDField("_id");

							deserialise(bson, buffer);
						}

						repo::lib::RepoUUID getUniqueId() const {
							return uniqueId;
						}

						std::uint32_t getNumFaces() const {
							return faces.size();
						}
						std::vector<repo::lib::repo_face_t>& getFaces()
						{
							return faces;
						}

						std::uint32_t getNumVertices() const {
							return vertices.size();
						}
						const std::vector<repo::lib::RepoVector3D>& getVertices() const {
							return vertices;
						}

						void bakeVertices(repo::lib::RepoMatrix transform) {
							for (int i = 0; i < vertices.size(); i++) {
								vertices[i] = transform * vertices[i];
							}
						}

						const std::vector<repo::lib::RepoVector3D>& getNormals() const
						{
							return normals;
						}

						const std::vector<std::vector<repo::lib::RepoVector2D>>& getUVChannelsSeparated() const
						{
							return channels;
						}

					private:
						void deserialise(
							repo::core::model::RepoBSON& bson,
							std::vector<uint8_t>& buffer)
						{
							auto blobRefBson = bson.getObjectField("_blobRef");
							auto elementsBson = blobRefBson.getObjectField("elements");

							if (elementsBson.hasField("vertices")) {
								auto vertBson = elementsBson.getObjectField("vertices");
								deserialiseVector(vertBson, buffer, vertices);
							}

							if (elementsBson.hasField("normals")) {
								auto vertBson = elementsBson.getObjectField("normals");
								deserialiseVector(vertBson, buffer, normals);
							}

							if (elementsBson.hasField("faces")) {

								int32_t faceCount = bson.getIntField("faces_count");
								faces.reserve(faceCount);

								std::vector<uint32_t> serialisedFaces = std::vector<uint32_t>();
								auto faceBson = elementsBson.getObjectField("faces");
								deserialiseVector(faceBson, buffer, serialisedFaces);

								// Retrieve numbers of vertices for each face and subsequent
								// indices into the vertex array.
								// In API level 1, mesh is represented as
								// [n1, v1, v2, ..., n2, v1, v2...]

								int mNumIndicesIndex = 0;
								while (serialisedFaces.size() > mNumIndicesIndex)
								{
									int mNumIndices = serialisedFaces[mNumIndicesIndex];
									if (serialisedFaces.size() > mNumIndicesIndex + mNumIndices)
									{
										repo::lib::repo_face_t face;
										face.resize(mNumIndices);
										for (int i = 0; i < mNumIndices; ++i)
											face[i] = serialisedFaces[mNumIndicesIndex + 1 + i];
										faces.push_back(face);
										mNumIndicesIndex += mNumIndices + 1;
									}
									else
									{
										repoError << "Cannot copy all faces. Buffer size is smaller than expected!";
									}
								}

							}

							if (elementsBson.hasField("uv_channels")) {
								std::vector<repo::lib::RepoVector2D> serialisedChannels;
								auto uvBson = elementsBson.getObjectField("uv_channels");
								deserialiseVector(uvBson, buffer, serialisedChannels);

								if (serialisedChannels.size())
								{
									//get number of channels and split the serialised.
									uint32_t nChannels = bson.getIntField(REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT);
									uint32_t vecPerChannel = serialisedChannels.size() / nChannels;
									channels.reserve(nChannels);
									for (uint32_t i = 0; i < nChannels; i++)
									{
										channels.push_back(std::vector<repo::lib::RepoVector2D>());
										channels[i].reserve(vecPerChannel);

										uint32_t offset = i * vecPerChannel;
										channels[i].insert(channels[i].begin(), serialisedChannels.begin() + offset,
											serialisedChannels.begin() + offset + vecPerChannel);
									}
								}
							}
						}

						template <class T>
						void deserialiseVector(
							repo::core::model::RepoBSON& bson,
							std::vector<uint8_t>& buffer,
							std::vector<T>& vec)
						{
							auto start = bson.getIntField("start");
							auto size = bson.getIntField("size");

							vec.resize(size / sizeof(T));
							memcpy(vec.data(), buffer.data() + (sizeof(uint8_t) * start), size);
						}
					};



					repo::lib::RepoUUID sharedId;
					std::uint32_t numVertices = 0;
					repo::lib::RepoUUID parent;
					repo::lib::RepoBounds bounds;

					std::unique_ptr<SupermeshingData> supermeshingData;


				public:
					StreamingMeshNode()
					{
						// Default constructor so instances can be initialised for vectors
					}

					StreamingMeshNode(repo::core::model::RepoBSON bson)
					{
						if (bson.hasField("shared_id")) {
							sharedId = bson.getUUIDField("shared_id");
						}
						if (bson.hasField("vertices_count")) {
							numVertices = bson.getIntField("vertices_count");
						}
						if (bson.hasField("parents")) {
							auto parents = bson.getUUIDFieldArray("parents");
							parent = parents[0];
						}
						if (bson.hasField("bounding_box")) {
							bounds = bson.getBoundsField("bounding_box");
						}
					}

					bool supermeshingDataLoaded() {
						return supermeshingData != nullptr;
					}

					void LoadSupermeshingData(repo::core::model::RepoBSON bson, std::vector<uint8_t>& buffer) {
						if (supermeshingDataLoaded())
						{
							// Throw warning
						}

						supermeshingData = std::make_unique<SupermeshingData>(bson, buffer);
					}

					void UnloadSupermeshingData() {
						supermeshingData.reset();
					}

					repo::lib::RepoUUID getSharedId() const {
						return sharedId;
					}

					std::uint32_t getNumVertices() const
					{
						return numVertices;
					}

					repo::lib::RepoBounds getBoundingBox() const
					{
						return bounds;
					}

					repo::lib::RepoUUID getParent() const
					{
						return parent;
					}

					void transformBounds(repo::lib::RepoMatrix transform) {
						auto newMinBound = transform * bounds.min();
						auto newMaxBound = transform * bounds.max();
						bounds = repo::lib::RepoBounds(newMinBound, newMaxBound);
					}

					// Requiring the supermeshing data to be loaded

					repo::lib::RepoUUID getUniqueId() {
						if (supermeshingDataLoaded()) {
							return supermeshingData->getUniqueId();
						}
						else {
							// Throw error
							throw std::exception("Supermesh data not loaded");
						}
					}

					std::uint32_t getNumFaces() {
						if (supermeshingDataLoaded()) {
							return supermeshingData->getNumFaces();
						}
						else {
							// Throw error
							throw std::exception("Supermesh data not loaded");
						}
					}

					std::vector<repo::lib::repo_face_t>& getFaces()
					{
						if (supermeshingDataLoaded()) {
							return supermeshingData->getFaces();
						}
						else {
							// TODO FT: Throw proper error			
							throw std::exception("Supermesh data not loaded");
						}
					}

					std::uint32_t getNumVertices() {
						if (supermeshingDataLoaded()) {
							return supermeshingData->getNumVertices();
						}
						else {
							// Throw error
							throw std::exception("Supermesh data not loaded");
						}
					}
					const std::vector<repo::lib::RepoVector3D>& getVertices() {
						if (supermeshingDataLoaded()) {
							return supermeshingData->getVertices();
						}
						else {
							// Throw error			
							throw std::exception("Supermesh data not loaded");
						}
					}

					void bakeVertices(repo::lib::RepoMatrix transform) {
						if (supermeshingDataLoaded()) {
							return supermeshingData->bakeVertices(transform);
						}
						else {
							// Throw error			
							throw std::exception("Supermesh data not loaded");
						}
					}

					const std::vector<repo::lib::RepoVector3D>& getNormals()
					{
						if (supermeshingDataLoaded()) {
							return supermeshingData->getNormals();
						}
						else {
							// Throw error			
							throw std::exception("Supermesh data not loaded");
						}
					}

					const std::vector<std::vector<repo::lib::RepoVector2D>>& getUVChannelsSeparated()
					{
						if (supermeshingDataLoaded()) {
							return supermeshingData->getUVChannelsSeparated();
						}
						else {
							// Throw error			
							throw std::exception("Supermesh data not loaded");
						}
					}
				};


				// TODO: Right location?
				typedef std::unordered_map <repo::lib::RepoUUID, std::shared_ptr<std::pair<repo::lib::RepoUUID, repo::lib::repo_material_t>>, repo::lib::RepoUUIDHasher> MaterialPropMap;
				typedef std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> TransformMap;

				std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher> getAllTransforms(
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
					std::string database,
					std::string collection,
					repo::lib::RepoUUID revId
				);

				void traverseTransformTree(
					const repo::core::model::RepoBSON root,
					const std::unordered_map<repo::lib::RepoUUID,
					std::vector<repo::core::model::RepoBSON>,
					repo::lib::RepoUUIDHasher> childNodeMap,
					std::unordered_map<repo::lib::RepoUUID,
					repo::lib::RepoMatrix, repo::lib::RepoUUIDHasher>& leafTransforms);

				MaterialPropMap& getAllMaterials(
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
					std::string database,
					std::string collection,
					repo::lib::RepoUUID revId
				);

				std::vector < repo::lib::RepoUUID> getAllTextureIds(
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
					std::string database,
					std::string collection,
					repo::lib::RepoUUID revId
				);

				void ProcessUntexturedGroup(
					const std::string database,
					const std::string collection,
					const repo::lib::RepoUUID revId,
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
					std::shared_ptr<repo::manipulator::modelconvertor::RepoBundleExport> exporter,
					const TransformMap& transformMap,
					const MaterialPropMap& matPropMap,
					const bool isOpaque,
					const int primitive);

				void ProcessTexturedGroup(
					const std::string database,
					const std::string collection,
					const repo::lib::RepoUUID revId,
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
					std::shared_ptr<repo::manipulator::modelconvertor::RepoBundleExport> exporter,
					const TransformMap& transformMap,
					const MaterialPropMap& matPropMap,
					const repo::lib::RepoUUID texId);

				std::vector<std::vector<std::shared_ptr<StreamingMeshNode>>> buildBVHAndCluster(
					const std::string database,
					const std::string collection,
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
					const TransformMap& transformMap,
					const repo::core::handler::database::query::RepoQuery& filter
				);

				void createSuperMeshes(
					const std::string database,
					const std::string collection,
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
					std::shared_ptr<repo::manipulator::modelconvertor::RepoBundleExport> exporter,
					const TransformMap& transformMap,
					const MaterialPropMap& matPropMap,
					std::vector<std::vector<std::shared_ptr<StreamingMeshNode>>>& clusters,
					repo::lib::RepoUUID texId = repo::lib::RepoUUID()
				);

				void createSuperMesh(
					std::shared_ptr<repo::manipulator::modelconvertor::RepoBundleExport> exporter,
					mapped_mesh_t& mappedMesh
				);

				std::vector<double> getWorldOffset(
					std::string database,
					std::string collection,
					repo::lib::RepoUUID revId,
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler);

				// New implementation above this line

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


				void appendMesh(					
					std::shared_ptr<StreamingMeshNode> node,
					const MaterialPropMap& matPropMap,
					mapped_mesh_t& mappedMesh,
					repo::lib::RepoUUID texId
				);

				Bvh buildFacesBvh(
					std::shared_ptr<StreamingMeshNode> node
				);

				void flattenBvh(
					const Bvh& bvh,
					std::vector<size_t>& leaves,
					std::vector<size_t>& branches
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
					std::shared_ptr<StreamingMeshNode> node,
					std::shared_ptr<repo::manipulator::modelconvertor::RepoBundleExport> exporter,
					const MaterialPropMap& matPropMap,
					repo::lib::RepoUUID texId
				);

				/*
				* Turns a mapped_mesh_t into a MeshNode that can be added to the database
				*/
				repo::core::model::SupermeshNode* createSupermeshNode(
					const mapped_mesh_t& mapped
				);


				/**
				* Groups the MeshNodes into sets based on their location and
				* geometry size.
				*/
				std::vector<std::vector<std::shared_ptr<StreamingMeshNode>>> clusterMeshNodes(
					const std::vector<std::shared_ptr<StreamingMeshNode>>& nodes
				);

				void clusterMeshNodesBvh(
					const std::vector<std::shared_ptr<StreamingMeshNode>>& meshes,
					std::vector<std::vector<std::shared_ptr<StreamingMeshNode>>>& clusters);

				Bvh buildBoundsBvh(
					const std::vector<std::shared_ptr<StreamingMeshNode>>& meshes
				);

				std::vector<size_t> getVertexCounts(
					const Bvh& bvh,
					const std::vector<std::shared_ptr<StreamingMeshNode>>& primitives
				);

				std::vector<size_t> getSupermeshBranchNodes(
					const Bvh& bvh,
					std::vector<size_t> vertexCounts);

				void splitBigClusters(
					std::vector<std::vector<std::shared_ptr<StreamingMeshNode>>>& clusters);
								
			};
		}
	}
}
