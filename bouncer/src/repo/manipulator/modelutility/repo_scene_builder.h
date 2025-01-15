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
#include "repo/core/handler/fileservice/repo_file_handler_abstract.h"
#include "repo/lib/datastructure/repo_structs.h"
#include "repo/lib/datastructure/repo_variant.h"

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

				// Call when no more nodes are expected.
				void finalise();

				repo::lib::RepoVector3D64 getWorldOffset();
				void setWorldOffset(const repo::lib::RepoVector3D64& offset);

				void setMissingTextures();
				bool hasMissingTextures();

			private:

				// All nodes will be committed with this as the revision id
				repo::lib::RepoUUID revisionId;

				std::string databaseName;
				std::string projectName;

				// Stored offset that should be applied to the scene when it is done.
				// Note that this doesn't affect nodes added with addNode - they must already
				// have this applied!
				repo::lib::RepoVector3D64 offset;

				// Indicates that one material node added is missing textures. This must be
				// set directly (i.e. it is not inferred from addNode). It is informational,
				// to pass onto Scene - this flag doesn't do anything to RepoSceneBuilder.
				bool isMissingTextures;

				std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler;

				std::vector<const repo::core::model::RepoNode*> nodesToCommit;

				// Commits all nodes in the nodesToCommit vector immediately
				void commitNodes();

				// Schedule a node to be comitted to the database. Once queued, the node
				// becomes immutable and must no longer be accessible outside the builder.
				void queueNode(const repo::core::model::RepoNode* node);

				struct Deleter;

				size_t referenceCounter;
			};
		}
	}
}