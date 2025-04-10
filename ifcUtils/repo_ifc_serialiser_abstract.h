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

#include <repo/manipulator/modelutility/repo_scene_builder.h>

namespace ifcUtils
{
	class AbstractIfcSerialiser
	{
	public:
		/*
		* Import all elements into the RepoSceneBuilder
		*/
		virtual void import(repo::manipulator::modelutility::RepoSceneBuilder* builder) = 0;

		virtual void setLevelOfDetail(int lod) = 0;

		/*
		* Maximum number of threads the importer should use for generating geometry.
		* 0 or less indicates the default number of threads should be used, which is
		* the same as the number of physical CPUs.
		*/
		virtual void setNumThreads(int numThreads) = 0;
	};
}