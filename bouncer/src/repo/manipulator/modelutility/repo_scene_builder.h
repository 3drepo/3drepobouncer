/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo/core/model/repo_model_global.h"
#include "repo/core/model/bson/repo_node.h"
#include "repo/core/handler/repo_database_handler_abstract.h"
#include "repo/core/handler/database/repo_query_fwd.h"
#include "repo/core/handler/fileservice/repo_file_handler_abstract.h"
#include "repo/lib/datastructure/repo_structs.h"
#include "repo/lib/datastructure/repo_variant.h"
#include "repo/manipulator/modelconvertor/import/repo_model_units.h"

#include <memory>
#include <vector>
#include <type_traits>

namespace repo {
	namespace core {
		namespace model {
			class MeshNode;
			class TransformationNode;
			class MetadataNode;
			class MaterialNode;
			class TextureNode;
		}
	}
}

namespace repo {
	namespace manipulator {
		namespace modelutility {

			/*
			* RepoSceneBuilder is used to build a RepoScene piece-wise directly to the
			* database. It should be used instead of the RepoScene factory method in
			* cases where RepoNodes are expected to consume prohibitive amounts of
			* memory, such as importers that are expected to handle large models.
			*/
			class REPO_API_EXPORT RepoSceneBuilder
			{
			public:

				using Metadata = const std::unordered_map<std::string, repo::lib::RepoVariant>;

				RepoSceneBuilder(
					std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
					const std::string& database,
					const std::string& project,
					const repo::lib::RepoUUID& revisionId);

				~RepoSceneBuilder();

				/*
				* To use RepoSceneBuilder, construct nodes using the RepoBSONFactory and
				* immediately hand them to RepoSceneBuilder using addNode.
				* 
				* AddNode adds a copy of the node to this Scene and returns a shared pointer
				* to the copy. Once the pointer goes out of scope, the node is scheduled for
				* committal and becomes immutable and inaccessible.
				*/

				template<repo::core::model::RepoNodeClass T>
				std::shared_ptr<T> addNode(const T& node);

				void addNode(std::unique_ptr<repo::core::model::RepoNode> node);

				/*
				* Adds nodes to the builder. These nodes can no longer be accessed once they
				* have been handed over.
				*/
				void addNodes(std::vector<std::unique_ptr<repo::core::model::RepoNode>> nodes);

				/**
				* Adds the provided sharedId as a parent to the node with the given uniqueId.
				* Nodes must already have been added with addNode.
				*/
				void addParent(repo::lib::RepoUUID nodeUniqueId, repo::lib::RepoUUID parentSharedId);

				// Call when no more nodes are expected.
				void finalise();

				repo::lib::RepoVector3D64 getWorldOffset();
				void setWorldOffset(const repo::lib::RepoVector3D64& offset);

				/*
				* Adds the repo_material_t to the scene with the specified parent, or adds
				* the parent Id to the material's existing node.
				*/
				void addMaterialReference(const repo_material_t& m, repo::lib::RepoUUID parentId);

				void setMissingTextures();
				bool hasMissingTextures();

				void setUnits(repo::manipulator::modelconvertor::ModelUnits units);
				repo::manipulator::modelconvertor::ModelUnits getUnits();

			private:

				template<typename Value>
				using RepoUUIDMap = std::unordered_map<repo::lib::RepoUUID, Value, repo::lib::RepoUUIDHasher>;

				void addTextureReference(std::string texture, repo::lib::RepoUUID parentId);
				std::unique_ptr<repo::core::model::TextureNode> createTextureNode(const std::string& texturePath);

				// All nodes will be committed with this as the revision id
				repo::lib::RepoUUID revisionId;

				repo::manipulator::modelconvertor::ModelUnits units;

				std::string databaseName;
				std::string projectName;

				std::string getSceneCollectionName();

				// Convenience member that tracks the total number of nodes queued for commit
				// over the life of this builder.
				size_t nodeCount;

				// Stored offset that should be applied to the scene when it is done.
				// Note that this doesn't affect nodes added with addNode - they must already
				// have this applied!
				repo::lib::RepoVector3D64 offset;

				// Indicates that one material node added is missing textures. This must be
				// set directly (i.e. it is not inferred from addNode). It is informational,
				// to pass onto Scene - this flag doesn't do anything to RepoSceneBuilder.
				bool isMissingTextures;

				std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler;

				// These lookups are for use by the addMaterialReference method, which will
				// update the parents of existing nodes.

				std::unordered_map<size_t, repo::lib::RepoUUID> materialToUniqueId;
				std::unordered_map<std::string, repo::lib::RepoUUID> textureToUniqueId;

				// We have to use raw pointers here because the std containers' interaction
				// with smart pointers requires the classes must be fully defined.

				RepoUUIDMap<repo::core::handler::database::query::AddParent*> parentUpdates;

				// Commits everything that might be outstanding, such as remaining updates,
				// to the async writer object.
				void commit();

				// Schedule a node to be comitted to the database. Once queued, the node
				// becomes immutable and must no longer be accessible outside the builder.
				void queueNode(repo::core::model::RepoNode* node);

				struct Deleter;

				size_t referenceCounter;

				// This is the multithreaded part of RepoSceneBuilder; it appears to the
				// outer-part as a concurrent queue. Internally it has a thread that owns a
				// database bulk write context.
				class AsyncImpl;
				std::unique_ptr<AsyncImpl> impl;
			};
		}
	}
}