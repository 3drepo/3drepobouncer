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
#include "geometry_utils.h"

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
					* Gets the geometry for the node in project coordinates, adding new geometry
					* to the IndexedMeshBuilder. All geometry read into the pipeline should be
					* via an IndexedMeshBuilder, because this is necessary for closed mesh
					* detection, which is a key part of both pipelines.
					*/
					static void loadGeometry(
						DatabasePtr handler, 
						Graph::Node& node,
						geometry::RepoIndexedMeshBuilder& builder
					);
				};
			}
		}
	}
}