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

#include "repo_model_export_x3d_abstract.h"
#include "../repo_model_export_abstract.h"
#include "../../../../lib/repo_property_tree.h"
#include "../../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{

			class X3DGLTFModelExport : public AbstractX3DModelExport
			{	
			public:

				/**
				* Export a Scene as a x3d file
				* @param scene scene to export
				*/
				X3DGLTFModelExport(
					const repo::core::model::RepoScene *scene
					);

				/**
				* Export a x3D file for a mesh seen as a scene on its own
				* @param mesh the mesh in question
				* @param scene repo scene as reference
				*/
				X3DGLTFModelExport(
					const repo::core::model::MeshNode  &mesh,
					const repo::core::model::RepoScene *scene);

				/**
				* Default Destructor
				*/
				virtual ~X3DGLTFModelExport() {};				
				
			protected:
				/**
				* Populate the given tree with properties associated with the node
				* @param node node in question
				* @param scene reference scene
				* @param tree tree to populate (the subtree will be populated from the root)
				* @return the subtree label that is required for the node type
				*/
				virtual std::string populateTreeWithProperties(
					const repo::core::model::RepoNode  *node,
					const repo::core::model::RepoScene *scene,
					repo::lib::PropertyTree            &tree
					);

				/**
				* Write the given scene as a x3dScene
				* @return returns true upon success
				*/
				virtual bool writeScene(
					const repo::core::model::RepoScene *scene);

				/**
				* Write the given mesh as a x3dScene
				* @return returns true upon success
				*/
				virtual bool writeMultiPartMeshAsScene(
					const repo::core::model::MeshNode &mesh,
					const repo::core::model::RepoScene *scene);
				
			};

		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo

