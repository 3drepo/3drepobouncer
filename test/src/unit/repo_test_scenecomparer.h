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
#include <set>
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
			* SceneComparer works by fingerprinting the contents of each node, and
			* combining these hashes of parent nodes with those of their children, bottom-
			* up.
			* If two branches are identical, then the hashes should be identical all the
			* way down. In this way the hashes can be used to determine the branch at
			* which two scenes diverge. This approach also makes SceneComparer very, very
			* sensitive however. Any minor difference in a node will cascade up to the
			* root.
			*
			* SceneComparer therefore can be difficult to use. The reported difference is
			* unlikely to be the actual differing node, but is most likely due to a
			* mismatch between an ancestor. The way to use SceneComparer to find the
			* difference is to use the ignore flags to isolate which characteristic of the
			* graph diverges, and go from there.
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
				* Ignoring an element of the scene means to consider it nonexistent. Ignored
				* Nodes are not loaded at all, and ignored members are set to their defaults.
				*/

				bool ignoreMaterials = false;

				bool ignoreMetadataNodes = false;

				bool ignoreMeshNodes = false;

				bool ignoreVertices = false;

				bool ignoreFaces = false;

				bool ignoreBounds = false;

				std::set<std::string> ignoreMetadataKeys;

				/*
				* Textures are not supported yet.
				*/
				bool ignoreTextures = true;

				struct Report
				{
					size_t nodesCompared = 0;
				};

				struct Result
				{
					std::string message;

					Report report;

					// Conversion operator that says whether the comparision has succeeded or not
					operator bool() const
					{
						return message.empty() && report.nodesCompared > 0;
					}
				};

				Result compare(std::string expectedDb, std::string expectedName, std::string actualDb, std::string actualName);

			private:
				std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler;

				struct Node;
				struct Scene;

				using Pair = std::tuple<Node*, Node*>;

				Report report;

				std::shared_ptr<SceneComparer::Scene> loadScene(std::string db, std::string name);
				void compare(Pair begin);
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
