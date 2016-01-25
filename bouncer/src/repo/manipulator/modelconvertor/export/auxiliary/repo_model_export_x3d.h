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
* Export functionality to X3Dom
* This export does not contain the actual binaries to render the model
* This provides a framework for other exports (like SRC) to be rendered
* within x3dom.
*/


#pragma once

#include <string>

#include "../repo_model_export_abstract.h"
#include "../../../../lib/repo_property_tree.h"
#include "../../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{

			class X3DModelExport
			{	
			public:
				/**
				* Default Constructor, export model with default settings
				* @param scene repo scene to convert
				*/
				X3DModelExport(
					const repo::core::model::MeshNode  &mesh,
					const repo::core::model::RepoScene *scene);

				/**
				* Default Destructor
				*/
				virtual ~X3DModelExport();				

				/**
				* Obtain the x3d representation as a buffer
				* @return returns a vector of uint8_t as a buffer
				*/
				std::vector<uint8_t> getFileAsBuffer() const;

				/**
				* Obtain the file name for this x3d document
				* @return returns the file name
				*/
				std::string getFileName() const;

				/**
				* Returns the status of the converter,
				* whether it has successfully converted the model
				* @return returns true if success
				*/
				bool isOk() const
				{
					return convertSuccess;
				}

				
			private:
				const repo::core::model::RepoScene *scene;
				std::string fname;
				repo::lib::PropertyTree tree;
				bool convertSuccess;
			
				/**
				* Write header of the dom XML onto
				* the property tree
				* @return returns true upon success
				*/
				bool includeHeader();

				/**
				* Populate the property tree with
				* Mesh information
				*/
				bool populateTree(
					const repo::core::model::MeshNode  &mesh,
					const repo::core::model::RepoScene *scene);

				/**
				* Write the given mesh as a x3dScene
				* @return returns true upon success
				*/
				bool writeMultiPartMeshAsScene(
					const repo::core::model::MeshNode &mesh,
					const repo::core::model::RepoScene *scene);

				/**
				* Calculates the centre of the box
				* @param min min. bounding box
				* @param max max. bounding box
				* @return returns the position of centre
				*/
				repo_vector_t getBoxCentre(
					const repo_vector_t &min,
					const repo_vector_t &max) const;

				/**
				* Calculates the size of the box
				* @param min min. bounding box
				* @param max max. bounding box
				* @return returns the size of box
				*/
				repo_vector_t getBoxSize(
					const repo_vector_t &min,
					const repo_vector_t &max) const;

			};

		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo

