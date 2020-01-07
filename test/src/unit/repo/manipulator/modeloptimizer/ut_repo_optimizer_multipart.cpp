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

#include <gtest/gtest.h>
#include <repo/manipulator/modeloptimizer/repo_optimizer_multipart.h>

using namespace repo::manipulator::modeloptimizer;

TEST(MultipartOptimizer, ConstructorTest)
{
	MultipartOptimizer();
}

TEST(MultipartOptimizer, DeconstructorTest)
{
	auto ptr = new TransformationReductionOptimizer();
	delete ptr;
}

TEST(MultipartOptimizer, ApplyOptimizationTest)
{

	auto opt = TransformationReductionOptimizer();
	repo::core::model::RepoScene *empty = nullptr;
	repo::core::model::RepoScene *empty2 = new repo::core::model::RepoScene();

	EXPECT_FALSE(opt.apply(empty));
	EXPECT_FALSE(opt.apply(empty2));

	//FIXME: to finish. need to construct a scene to test this properly.

}

