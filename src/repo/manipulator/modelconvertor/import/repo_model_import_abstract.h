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
* Abstract Model convertor(Import)
*/


#pragma once

#include <string>

#include "repo_model_import_config.h"
#include "../../graph/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			class AbstractModelImport
			{
			public:
				/**
				* Default Constructor, generate model with default settings
				*/
				AbstractModelImport();

				/**
				* Create AbstractModelImport with specific settings
				* NOTE: The destructor will destroy the settings object referenced
				* in this object!
				* @param settings
				*/
				AbstractModelImport(const ModelImportConfig *settings);

				/**
				* Default Deconstructor
				* NOTE: The destructor will destroy the settings object referenced
				* in this object!
				*/
				virtual ~AbstractModelImport();


				/**
				* Generates a repo scene graph
				* an internal representation needs to have
				* been created before this call (e.g. by means of importModel())
				* @return returns a populated RepoScene upon success.
				*/
				virtual repo::manipulator::graph::RepoScene* generateRepoScene() = 0;

				/**
				* Import model from a given file
				* This does not generate the Repo Scene Graph
				* Use getRepoScene() to generate a Repo Scene Graph. 
				* @param path to the file
				* @param error message if failed
				* @return returns true upon success
				*/
				virtual bool importModel(std::string filePath, std::string &errMsg) = 0;

			protected:
				/**
				* Retrieve the directory to the file
				* e.g /home/user/file.txt would return /home/user
				* @param full path of the file
				* @return returns the path of the directory
				*/
				std::string  getDirPath(std::string fullPath);

				/**
				* Retrieve the filename from a full path
				* e.g /home/user/file.txt would return file.txt
				* @param full path of the file
				* @return returns the file name
				*/
				std::string  getFileName(std::string fullPath);

				const ModelImportConfig *settings; /*! Stores related settings for model import */
				bool destroySettings; //only destroy settings if it is constructed by this object

			};

		} //namespace AbstractModelImport
	} //namespace manipulator
} //namespace repo

