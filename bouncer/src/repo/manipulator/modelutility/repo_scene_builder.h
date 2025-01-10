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
			class RepoNode;
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

			template<class T>
			concept RepoNodeClass = std::is_base_of<repo::core::model::RepoNode, T>::value;

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
					std::string database,
					std::string project);

				~RepoSceneBuilder();

				/*
				* To use RepoSceneBuilder, construct nodes using the RepoBSONFactory and
				* immediately hand them to RepoSceneBuilder using addNode.
				* 
				* AddNode adds a copy of the node to this Scene and returns a shared pointer
				* to the copy. Once the pointer goes out of scope, the node is scheduled for
				* committal and becomes immutable and inaccessible.
				*/

				template<RepoNodeClass T>
				std::shared_ptr<T> addNode(const T& node);

				// Call when no more nodes are expected.
				void finalise();

			private:

				// All nodes will be committed with this as the revision id
				repo::lib::RepoUUID revisionId;

				std::string databaseName;
				std::string projectName;

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

			/*
			* The MaterialCache is used with RepoSceneBuilder to track references to
			* MaterialNodes.
			* Material and Texture Nodes are special in the sense that it is much more
			* likely that they will be referenced multiple times over an import.
			* 
			* Right now, MaterialCache just holds pointers to keep materials from being
			* collected, but in the future through a dedicated operation to update the
			* parentIds, it could let the nodes be committed.
			* 
			* Internally, nodes are indexed through a checksum, based on their properties.
			* We assume there are no collisions, by virtue of the simplicity of the
			* material parameters.
			*/
			class MaterialCache
			{
			public:
				//void addMaterialReference(repo_material_t material, repo::li)

			private:
				std::unordered_map<size_t, std::shared_ptr<repo::core::model::MaterialNode>> materials;
				std::unordered_map<size_t, std::shared_ptr<repo::core::model::TextureNode>> textures;
			};
		}
	}
}