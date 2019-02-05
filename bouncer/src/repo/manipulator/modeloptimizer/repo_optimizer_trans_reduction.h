/**
*  Copyright (C) 2015 3D Repo Ltd
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
* Transformation Reduction Optimizer
* Reduces the amount of transformations within the scene graph by
* merging transformation with its mesh (provided the mesh doesn't have
* multiple parent
*/

#include "repo_optimizer_abstract.h"

#include "../../core/model/bson/repo_node_camera.h"
#include "../../core/model/bson/repo_node_mesh.h"

#include <tuple>
#include <unordered_map>

typedef std::tuple<repo::core::model::RepoScene::GraphType, repo::lib::RepoUUID, repo::core::model::NodeType> RepoCacheTuple;

namespace repo {
	namespace manipulator {
		namespace modeloptimizer {

			class TransformationReductionOptimizer : AbstractOptimizer{
			public:
				/**
				* Default constructor
				* if strictMode is set to true, only single trans
				* to single meta/mesh will be optimised
				*/
				TransformationReductionOptimizer(
					const bool strictMode = false);

				/**
				* Default deconstructor
				*/
				virtual ~TransformationReductionOptimizer();

				/**
				* Apply optimisation on the given repoScene
				* @param scene takes in a repoScene to optimise
				* @return returns true upon success
				*/
				virtual bool apply(repo::core::model::RepoScene *scene);

			private:
				const repo::core::model::RepoScene::GraphType gType;
				const bool strictMode;

				/**
				* Apply optimization on the mesh, given
				* it satisfies the condition for the optimisation to happen
				* @param scene scene to optimise
				* @param mesh mesh in question
				*/
				void applyOptimOnMesh(
					repo::core::model::RepoScene *scene,
					repo::core::model::MeshNode  *mesh);

				/**
				* Apply optimization on the camera, given
				* it satisfies the condition for the optimisation to happen
				* @param scene scene to optimise
				* @param camera camera in question
				*/
				void applyOptimOnCamera(
					repo::core::model::RepoScene *scene,
					repo::core::model::CameraNode  *camera);
			};
		}
	}
}
