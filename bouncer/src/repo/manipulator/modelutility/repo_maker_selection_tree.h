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
#include "repo/lib/repo_property_tree.h"
#include "repo/manipulator/modelutility/rapidjson/fwd.h"

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
					std::vector<repo::lib::RepoUUID> path;
					repo::lib::RepoUUID _id;
					repo::lib::RepoUUID shared_id;
					std::vector<Node> children;
					std::vector<repo::lib::RepoUUID> meta;

					enum ToggleState {
						SHOW,
						HIDDEN,
						HALF_HIDDEN
					};

					ToggleState toggleState;
				};

				Container container;
				Node nodes;

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
					const repo::core::model::RepoScene *scene);
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

				/**
				* Recurse function to generate property tree for a specific node
				* @param currentNode node to parse
				* @param idMap a map of pairs of id to name mapping (to insert)
				* @param currentPath the current path that leads to this node.
				* @param hiddenOnDefault (return value) shows if the subtree has hidden nodes
				* @param hiddenNode A list of vector of nodes that are hidden by default
				*/
				SelectionTree::Node generatePTree(
					const repo::core::model::RepoNode			*currentNode,
					std::unordered_map<std::string, std::pair<std::string, SelectionTree::ChildPath>>	&idMaps,
					IdMap										&uniqueIdToSharedId,
					IdToMeshes									&idToMeshes,
					std::vector<repo::lib::RepoUUID>            currentPath,
					bool                                        &hiddenOnDefault,
					std::vector<repo::lib::RepoUUID>			&hiddenNode,
					std::vector<repo::lib::RepoUUID>			&meshIds) const;

				SelectionTreesSet trees;

				void generateSelectionTrees();
			};
		}
	}
}
