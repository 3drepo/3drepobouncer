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

#include "repo_optimizer_trans_reduction.h"

using namespace repo::manipulator::modeloptimizer;

TransformationReductionOptimizer::TransformationReductionOptimizer() : AbstractOptimizer()
{
}


TransformationReductionOptimizer::~TransformationReductionOptimizer() 
{
}

bool TransformationReductionOptimizer::apply(repo::core::model::RepoScene *scene){
	bool success = false;
	if (scene)
	{

	}
	else
	{
		repoError << "Trying to apply transformation on an empty scene!";
	}
	return success;
}