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

#include <repo/core/model/collection/repo_scene.h>

namespace testing {

	class SceneHelper
	{
	public:
		repo::core::model::RepoScene* scene;

		SceneHelper(repo::core::model::RepoScene* scene) :
			scene(scene)
		{
		}

		struct NodeInfo
		{
			size_t numVisibleChildren;
			size_t numMeshes;

			NodeInfo(repo::core::model::RepoNode* node, SceneHelper* scene) :
				numVisibleChildren(0),
				numMeshes(0),
				scene(scene),
				node(node)
			{
			}

			bool isLeaf() {
				return !numVisibleChildren;
			}

			bool hasGeometry() {
				return numMeshes;
			}

			std::string name()
			{
				return node->getName();
			}

			SceneHelper* scene;
			repo::core::model::RepoNode* node;

			std::vector<NodeInfo> getChildren()
			{
				return scene->getChildNodes(node, true);
			}
		};

		std::vector<NodeInfo> findNodesByMetadata(std::string key, repo::lib::RepoVariant value);
		std::vector<NodeInfo> findTransformationNodesByName(std::string name);
		std::vector<NodeInfo> getChildNodes(repo::core::model::RepoNode* node, bool ignoreMeta);
		NodeInfo getNodeInfo(repo::core::model::RepoNode* node);
	};
}