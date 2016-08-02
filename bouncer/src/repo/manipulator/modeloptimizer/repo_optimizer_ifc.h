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
* Abstract optimizer class
*/

#pragma once

#include "../../core/model/collection/repo_scene.h"
#include "repo_optimizer_abstract.h"

const static std::string IFC_TYPE_SPACE_LABEL = "(IFC Space)";

namespace repo {
	namespace manipulator {
		namespace modeloptimizer {
			class IFCOptimzer : public AbstractOptimizer
			{
			public:
				/**
				* Default constructor
				*/
				IFCOptimzer();

				/**
				* Default deconstructor
				*/
				virtual ~IFCOptimzer();

				/**
				* Apply optimisation on the given repoScene
				* @param scene takes in a repoScene to optimise
				* @return returns true upon success
				*/
				virtual bool apply(repo::core::model::RepoScene *scene);

			private:

				/**
				* Remove Transformations that contains the names given
				* @param scene takes in a repoScene to optimise
				* @param transByName mapping of transformation and their name (capitalized)
				* @param names list of names to remove
				* @return true upon success
				*/
				bool removeTransformationsWithNames(
					repo::core::model::RepoScene *scene,
					const std::unordered_map<std::string, std::vector<repo::core::model::RepoNode*>>  &transByName,
					const std::vector<std::string> &names);

				/**
				* Sanitise the transformations to remove IFC specific keywords
				* @param scene scene to examine
				* @return true upon success
				*/
				bool sanitiseTransformationNames(
					repo::core::model::RepoScene *scene);
			};
		}
	}
}
