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
* Abstract Model convertor(Export)
*/


#pragma once

#include <string>

#include "../../graph/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			class AbstractModelExport
			{
			public:
				/**
				* Default Constructor, export model with default settings
				*/
				AbstractModelExport();

				/**
				* Default Deconstructor
				*/
				virtual ~AbstractModelExport();

				/**
				* Export a repo scene graph to file
				* @param scene repo scene representation
				* @param filePath path to destination file
				* @return returns true upon success
				*/
				virtual bool exportToFile(
					const repo::manipulator::graph::RepoScene *scene,
					const std::string &filePath) = 0;

			};

		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo

