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
			class AbstractX3DModelExport
			{
			public:

				/**
				* Export a Scene as a x3d file
				* @param scene scene to export
				*/
				AbstractX3DModelExport(
					const repo::core::model::RepoScene *scene
					);

				/**
				* Export a x3D file for a mesh seen as a scene on its own
				* @param mesh the mesh in question
				* @param scene repo scene as reference
				*/
				AbstractX3DModelExport(
					const repo::core::model::MeshNode  &mesh,
					const repo::core::model::RepoScene *scene);

				/**
				* Default Destructor
				*/
				virtual ~AbstractX3DModelExport() {};

				/**
				* Obtain the x3d representation as a buffer
				* @return returns a vector of uint8_t as a buffer
				*/
				std::vector<uint8_t> getFileAsBuffer();

				/**
				* Obtain the file name for this x3d document
				* @return returns the file name
				*/
				std::string getFileName() const;

				void initialize()
				{
					if (!initialised && sceneValid())
					{
						if (fullScene)
							convertSuccess = populateTree(scene);
						else
							convertSuccess = populateTree(mesh, scene);
					}

					initialised = true;
				}

				/**
				* Returns the status of the converter,
				* whether it has successfully converted the model
				* @return returns true if success
				*/
				bool isOk()
				{
					if (!initialised) initialize();
					return convertSuccess;
				}

				/**
				* Check if scene is valid for export
				* @return returns true if valid
				*/
				bool sceneValid();

			protected:
				const repo::core::model::RepoScene *scene;
				const repo::core::model::MeshNode &mesh;
				repo::core::model::RepoScene::GraphType gType;
				std::string fname;
				repo::lib::PropertyTree tree;
				bool convertSuccess, fullScene, initialised;

				/**
				* Create a subTree for google map tiles
				* @param mapNode with the information on the map tiles
				* @return returns a property tree with google map info
				*/
				repo::lib::PropertyTree createGoogleMapSubTree(
					const repo::core::model::MapNode *mapNode);

				/**
				* Generate the default viewing point for the scene
				* @return returns a property tree with default view point
				*/
				repo::lib::PropertyTree generateDefaultViewPointTree();

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
					) = 0;

				/**
				* Create a subtree for the subgraph
				* @param node node where this subgraph starts
				* @param gtype type of graph (expected to be optimized graph)
				* @param scene scene for reference
				* @return returns a property tree filled with the subGraph's info
				*/
				repo::lib::PropertyTree generateSubTree(
					const repo::core::model::RepoNode             *node,
					const repo::core::model::RepoScene::GraphType &gtype,
					const repo::core::model::RepoScene            *scene);

				/**
				* Write header of the dom XML onto
				* the property tree
				* @return returns true upon success
				*/
				bool includeHeader();

				/**
				* Populate the property tree with scene information
				* @param scene scene for reference
				* @return returns true upon success
				*/
				bool populateTree(
					const repo::core::model::RepoScene *scene);

				/**
				* Populate the property tree with
				* Mesh as the scene
				* @param mesh mesh in question
				* @param scene scene for reference
				* @return returns true upon success
				*/
				bool populateTree(
					const repo::core::model::MeshNode  &mesh,
					const repo::core::model::RepoScene *scene);

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
					const repo::core::model::RepoScene *scene) = 0;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
