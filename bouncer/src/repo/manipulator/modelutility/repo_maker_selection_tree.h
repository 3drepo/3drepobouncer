/**
*  Copyright (C) 2016 3D Repo Ltd
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
#include "repo/core/model/collection/repo_scene.h"
#include "repo/core/handler/repo_database_handler_abstract.h"

namespace repo{
	namespace manipulator{
		namespace modelutility{
			class SelectionTree
			{
			public:
				class Container {
				public:
					std::string account;
					std::string project;
				};

				class Node {
				public:
					repo::core::model::NodeType type;
					std::string name;
					repo::lib::RepoUUID _id;
					repo::lib::RepoUUID shared_id;
					std::vector<repo::lib::RepoUUID> meta;
					std::vector<Node*> children; // Use pointers here because references are not assignable so cant be used in a vector
					std::vector<repo::lib::RepoUUID> path;

					enum ToggleState {
						SHOW,
						HIDDEN,
						HALF_HIDDEN
					};

					ToggleState toggleState;

					Node() :
						toggleState(ToggleState::SHOW),
						type(repo::core::model::NodeType::UNKNOWN)
					{
					}
				};

				Container container;
				Node* root;
				std::vector<Node> nodes; // This vector holds the memory containing the actual nodes

				using ChildPath = std::vector<repo::lib::RepoUUID>;
			};

			using IdToName = std::unordered_map<repo::lib::RepoUUID, std::string, repo::lib::RepoUUIDHasher>;

			using IdToPath = std::unordered_map<repo::lib::RepoUUID, SelectionTree::ChildPath, repo::lib::RepoUUIDHasher>;

			using IdToMeshes = std::unordered_map<repo::lib::RepoUUID, std::vector<repo::lib::RepoUUID>, repo::lib::RepoUUIDHasher>;

			using IdMap = std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>;

			class Settings
			{
			public:
				std::vector<repo::lib::RepoUUID> hiddenNodes;
			};

			struct SelectionTreesSet
			{
				SelectionTree fullTree;
				IdToPath idToPath;
				IdToMeshes idToMeshes;
				IdToName idToName;
				IdMap idMap;
				Settings modelSettings;
			};

			class SelectionTreeMaker
			{
			public:
				/**
				* Create a selection tree maker.
				* The scene must be a valid pointer with Default graph loaded
				* @params scene scene to construct from
				*/
				SelectionTreeMaker(
					const repo::core::model::RepoScene *scene, repo::core::handler::AbstractDatabaseHandler* handler);
				~SelectionTreeMaker();

				/**
				* Construct and return the selection tree as a Property Tree As a buffer
				* The method will return an empty tree if the scene is null
				* or the default graph is not loaded.
				* @return returns the selection tree as a property tree
				*/
				std::map<std::string, std::vector<uint8_t>> getSelectionTreeAsBuffer() const;

			private:
				const repo::core::model::RepoScene *scene;
				repo::core::handler::AbstractDatabaseHandler* handler;
				SelectionTreesSet trees;

				void generateSelectionTrees();
			};
		}
	}
}
