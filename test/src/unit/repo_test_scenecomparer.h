/*
*  Copyright(C) 2025 3D Repo Ltd
*
*  This program is free software : you can redistribute it and / or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or(at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <memory>
#include <repo/core/handler/repo_database_handler_mongo.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/core/model/bson/repo_node_transformation.h>
#include <repo/core/model/bson/repo_node_metadata.h>
#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_node_material.h>
#include <repo/core/model/bson/repo_node_texture.h>

namespace repo {
	namespace test {
		namespace utils {
			/*
			* This class compares two scene graphs to check if they have the same content.
			* The class considers only deterministic fields, such as names and geometry,
			* but is independent of non-deterministic fields such as UUIDs or timestamps.
			* This class assumes that the scene collection contains only one revision.
			*
			* The algorithm to compare scenes attempts to match nodes between the two trees.
			*
			* Take care that the reported differences between nodes may be due to a mismatch
			* between the two trees: if a property change causes the relative position of
			* nodes to change with respect to the other tree, then the comparison may
			* encounter these differences first. This is most likely to occur when comparing
			* meshes with no names.
			*/
			class SceneComparer
			{
			public:
				SceneComparer();

				/*
				* The allowed deviation between any of the six components when comparing two
				* bounding boxes.
				*/
				double boundingBoxTolerance = 0.000001;

				/*
				* Don't consider vertices when comparing MeshNodes - this only applies to
				* vertices themselves, other flags exist for UVs and Normals, etc.
				*/
				bool ignoreVertices = false;

				/*
				* Do not consider key-value pairs when comparing metadata nodes. Correspdonding
				* metadata nodes with the same names must still exist.
				*/
				bool ignoreMetadataContent = false;

				/*
				* Textures are not supported yet.
				*/
				bool ignoreTextures = true;

				/*
				* Do not consider Material Nodes - it will be as if there are no MaterialNodes
				* in the scene.
				*/
				bool ignoreMaterials = true;

				struct Result
				{
					std::string message;

					// Conversion operator that says whether the comparision has succeeded or not
					operator bool() const
					{
						return message.empty();
					}
				};

				Result compare(std::string expectedDb, std::string expectedName, std::string actualDb, std::string actualName);

			private:
				std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler;

				struct Node;
				struct Scene;

				using Pair = std::tuple<Node*, Node*>;

				std::shared_ptr<SceneComparer::Scene> loadScene(std::string db, std::string name);
				void compare(Pair begin);
				template <typename T>
				static int compareNodeTypes(const Node* a, const Node* b);
				void compare(std::shared_ptr<repo::core::model::RepoNode> expected, std::shared_ptr<repo::core::model::RepoNode> actual);
				bool compare(const repo::lib::RepoBounds& a, const repo::lib::RepoBounds& b);
				void compareTransformationNode(std::shared_ptr<repo::core::model::TransformationNode> expected, std::shared_ptr<repo::core::model::TransformationNode> actual);
				void compareMeshNode(std::shared_ptr<repo::core::model::MeshNode> expected, std::shared_ptr<repo::core::model::MeshNode> actual);
				void compareMetaNode(std::shared_ptr<repo::core::model::MetadataNode> expected, std::shared_ptr<repo::core::model::MetadataNode> actual);
				void compareMaterialNode(std::shared_ptr<repo::core::model::MaterialNode> expected, std::shared_ptr<repo::core::model::MaterialNode> actual);

			};
		}
	}
}
