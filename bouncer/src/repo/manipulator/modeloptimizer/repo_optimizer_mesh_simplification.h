/**
*  Copyright (C) 2023 3D Repo Ltd
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
* Mesh Simplification Optimizer
* Reduces the number of primitives in each mesh individiually in order to save
* space and time storing, communicating and rendering the scene.
*/

#include "repo_optimizer_abstract.h"

#include "../../core/model/bson/repo_node_mesh.h"

#include <tuple>
#include <unordered_map>

typedef std::tuple<repo::core::model::RepoScene::GraphType, repo::lib::RepoUUID, repo::core::model::NodeType> RepoCacheTuple;

namespace repo {
	namespace manipulator {
		namespace modeloptimizer {
			class MeshSimplificationOptimizer : AbstractOptimizer {
			public:
				/**
				* The quality metric defines the target density of a mesh - how
				* many primitives there should be per unit volume.
				*/
				MeshSimplificationOptimizer(
					const double quality, const int minVertexCount);

				/**
				* Default deconstructor
				*/
				virtual ~MeshSimplificationOptimizer();

				/**
				* Apply optimisation on the given repoScene
				* @param scene takes in a repoScene to optimise
				* @return returns true upon success
				*/
				virtual bool apply(repo::core::model::RepoScene *scene);

			private:
				const repo::core::model::RepoScene::GraphType gType;

				/**
				* The target number of vertices per-unit cubed (in whichever
				* units the vertices are defined in).
				*/
				const double quality;

				/**
				* The minimum number of absolulte vertices in the mesh for 
				* simplification to be considered. Meshes with fewer vertices
				* will not be decimated.
				*/
				const int minVertexCount;

				/**
				* Defines a local mesh description used for the simplification
				*/
				class Mesh;

				/**
				* Converts a Repo MeshNode into the mutable representation for
				* simplification.
				*/
				void convertMeshNode(repo::core::model::MeshNode* node, Mesh& mesh);

				/**
				* Converts a mutable simplification mesh into a Repo MeshNode.
				* The MeshNode will be created with the default constructor.
				*/
				repo::core::model::MeshNode updateMeshNode(repo::core::model::MeshNode* node, Mesh& mesh);

				/**
				* Returns whether it is possible to run decimation on the mesh
				* node. Only meshes with specific attributes and topologies can
				* be decimated.
				*/
				bool canOptimizeMeshNode(repo::core::model::RepoScene* scene, repo::core::model::MeshNode* node);

				/**
				* Returns whether the mesh node meets the criteria for 
				* decimation. This check includes canOptimizeMeshNode.
				*/
				bool shouldOptimizeMeshNode(repo::core::model::RepoScene* scene, repo::core::model::MeshNode* node);

				/**
				* Simplifies the MeshNode mesh and returns a replacement MeshNode
				* that should be used in its place.
				*/
				repo::core::model::MeshNode optimizeMeshNode(repo::core::model::MeshNode* mesh);

				/**
				* Returns the volume of the mesh based on the bounding box.
				*/
				static double getVolume(repo::core::model::MeshNode* mesh);

				/**
				* Returns a metric defining the quality of the mesh. Currently
				* this is the number of vertices per unit volume.
				*/
				static double getQuality(repo::core::model::MeshNode* mesh);
			};
		}
	}
}
