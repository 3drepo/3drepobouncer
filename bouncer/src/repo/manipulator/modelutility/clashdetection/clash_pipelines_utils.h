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

#include "clash_pipelines.h"

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				/*
				* Contains a set of helper functions that operate with the types used to
				* construct the	clash detection pipelines.
				*/

				struct PipelineUtils 
				{
					/*
					* Gets the geometry for the node in project coordinates
					*/
					static void loadGeometry(
						DatabasePtr handler, 
						Graph::Node& node,
						std::vector<repo::lib::RepoVector3D64>& vertices,
						std::vector<repo::lib::repo_face_t>& faces
					);
				};
			}
		}
	}
}