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
#include <repo/core/model/bson/repo_node_mesh.h>

namespace testing {

	class SceneUtils
	{
	public:
		const repo::core::model::RepoScene* scene;

		SceneUtils(const repo::core::model::RepoScene* scene) :
			scene(scene)
		{
		}

		using Filter = std::initializer_list<repo::core::model::NodeType>;

		struct NodeInfo
		{
			size_t numVisibleChildren;
			size_t numMeshes;

			NodeInfo(repo::core::model::RepoNode* node, SceneUtils* scene) :
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

			std::string name() const
			{
				return node->getName();
			}

			NodeInfo getParent() const;

			std::vector<NodeInfo> getParents(Filter parents) const;

			SceneUtils* scene;
			repo::core::model::RepoNode* node;

			std::vector<NodeInfo> getChildren(Filter children)
			{
				return scene->getChildNodes(node, children);
			}

			/* Names of all children (excluding metadata); children that don't
			* have names are omitted (as opposed to being included as empty 
			strings */
			std::vector<std::string> getChildNames();

			std::vector<NodeInfo> getMeshes();

			std::vector<NodeInfo> getMeshes(repo::core::model::MeshNode::Primitive);

			std::vector<NodeInfo> getMeshesRecursive();

			repo::core::model::MeshNode getMeshInProjectCoordinates();

			std::vector<repo::core::model::MeshNode> getMeshesInProjectCoordinates();

			repo::lib::RepoBounds getProjectBounds();

			std::vector<NodeInfo> getTextures();

			std::vector<NodeInfo> getSiblings(Filter parents, Filter siblings);

			bool hasTextures();

			std::unordered_map<std::string, repo::lib::RepoVariant> getMetadata();

			std::vector<NodeInfo> getMetadataNodes();

			repo::lib::repo_material_t getMaterial();

			/*
			* Returns the colours of all the repo_material_t's that belong to the MeshNodes
			* directly below this node.
			*/
			std::vector<repo::lib::repo_color4d_t> getColours();

			bool hasTransparency();

			std::string getPath() const;

			repo::lib::RepoUUID getUniqueId() const
			{
				return node->getUniqueID();
			}

			repo::lib::RepoUUID getSharedId() const
			{
				return node->getSharedID();
			}

            bool operator==(const NodeInfo& other) const
            {
				return node->getUniqueID() == other.node->getUniqueID();
            }

            bool operator!=(const NodeInfo& other) const
            {
				return !(*this == other);
            }
		};

		std::vector<NodeInfo> findNodesByMetadata(std::string key, std::string value);
		NodeInfo findNodeByMetadata(std::string key, std::string value);
		NodeInfo findTransformationNodeByName(std::string name);
		NodeInfo findLeafNode(std::string name);
		NodeInfo findNodeByUniqueId(repo::lib::RepoUUID uniqueId);
		std::vector<NodeInfo> findLeafNodes(std::string name);
		std::vector<NodeInfo> findTransformationNodesByName(std::string name);
		std::vector<NodeInfo> getChildNodes(repo::core::model::RepoNode* node, Filter filter);
		std::vector<NodeInfo> getParentNodes(repo::core::model::RepoNode* node, Filter filter);
		std::vector<NodeInfo> getMeshes();
		std::vector<NodeInfo> getTransformations();
		std::vector<NodeInfo> getMetadataNodes();
		std::vector<NodeInfo> getReferenceNodes();
		repo::lib::RepoMatrix getWorldTransform(repo::core::model::RepoNode* node);
		NodeInfo getNodeInfo(repo::core::model::RepoNode* node);
		NodeInfo getRootNode();
		std::string getTeamspaceName();
		std::string getContainerName();

		bool isPopulated();
	};
}