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
* Allows Export functionality from 3D Repo World to SRC
*/


#pragma once

#include <string>

#include "repo_model_export_abstract.h"
#include "../../../lib/repo_property_tree.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{

			class REPO_API_EXPORT GLTFModelExport : public AbstractModelExport
			{	
			public:
				/**
				* Default Constructor, export model with default settings
				* @param scene repo scene to convert
				*/
				GLTFModelExport(const repo::core::model::RepoScene *scene);

				/**
				* Default Destructor
				*/
				virtual ~GLTFModelExport();

				//temporary function to debug gltf. to remove once done
				void GLTFModelExport::debug() const;

				/**
				* Export a repo scene graph to file
				* @param filePath path to destination file
				* @return returns true upon success
				*/
				virtual bool exportToFile(
					const repo::core::model::RepoScene *scene,
					const std::string &filePath){
					return false;
				};
				
				/**
				* Get supported file formats for this exporter
				*/
				static std::string getSupportedFormats()
				{
					return std::string();
				}

				/**
				* Returns the status of the converter,
				* whether it has successfully converted the model
				* @return returns true if success
				*/
				bool isOk() const
				{
					return convertSuccess;
				}

				/**
				* @param filePath path to destination file (including file extension)
				* @return returns true upon success
				*/
				bool writeSceneToFile(
					const std::string &filePath) {
					return false;
				}
				
			private:
				const repo::core::model::RepoScene *scene;
				bool convertSuccess;
				std::unordered_map<std::string, repo::lib::PropertyTree> trees;
				repo::core::model::RepoScene::GraphType gType;
				std::unordered_map<std::string, std::vector<uint8_t>> fullDataBuffer;


				/**
				* Add buffer into export (generic, not expected to be called directly)
				* @param name name of the buffer
				* @param fileName name of binary file
				* @param tree tree to insert header info into
				* @param buffer buffer to export
				* @param count number of elements in the buffer
				* @param byteLength number of bytes in the buffer
				* @param stride stride in bytes between elements
				* @param bufferTarget Target type as of OpenGL
				* @param componentType component type (SHORT, USHORT, FLOAT etc.) of the array
				* @param bufferType element type (SCALAR, VEC3D etc..)
				*/
				void GLTFModelExport::addBuffer(
					const std::string              &name,
					const std::string              &fileName,
					repo::lib::PropertyTree        &tree,
					const uint8_t                  *buffer,
					const size_t                   &count,
					const size_t                   &byteLength,
					const size_t                   &stride,
					const uint32_t                 &bufferTarget,
					const uint32_t                 &componentType,
					const std::string              &bufferType);

				/**
				* Add buffer into export (unsigned 16 bit integers)
				* @param name name of the buffer
				* @param fileName name of binary file
				* @param tree tree to insert header info into
				* @param buffer buffer to export
				*/
				void addBuffer(
					const std::string              &name,
					const std::string              &fileName,
					repo::lib::PropertyTree        &tree,
					const std::vector<uint16_t>    &buffer);

				/**
				* Add buffer into export (2x 32 bit float vectors)
				* @param name name of the buffer
				* @param fileName name of binary file
				* @param tree tree to insert header info into
				* @param buffer buffer to export
				*/
				void addBuffer(
					const std::string                   &name,
					const std::string                   &fileName,
					repo::lib::PropertyTree             &tree,
					const std::vector<repo_vector2d_t>    &buffer);

				/**
				* Add buffer into export (3x 32 bit float vectors)
				* @param name name of the buffer
				* @param fileName name of binary file
				* @param tree tree to insert header info into
				* @param buffer buffer to export
				*/
				void addBuffer(
					const std::string                   &name,
					const std::string                   &fileName,
					repo::lib::PropertyTree             &tree,
					const std::vector<repo_vector_t>    &buffer);
				

				/**
				* Add face buffer into the export
				* @param name name of the buffer
				* @param tree tree to insert header info into
				* @param faces the face buffer to export
				*/
				void addFaceBuffer(
					const std::string              &name,
					const std::string              &fileName,
					repo::lib::PropertyTree        &tree, 
					const std::vector<repo_face_t> &faces);

				/**
				* Construct JSON document about the scene
				* @param tree tree to place the info
				* @return returns true upon success
				*/
				bool GLTFModelExport::constructScene(
					repo::lib::PropertyTree &tree);

				/**
				* Create a tree representation for the graph
				* This creates the header of the SRC
				* @return returns true upon success
				*/
				bool generateTreeRepresentation();

				/**
				* Process children of nodes(Transformation)
				* Recurse call of populateWithNode() if there is a transformation as child
				* and also property lists (meshes/cameras)
				* @param node node in question
				*/
				void processNodeChildren(
					const repo::core::model::RepoNode *node,
					repo::lib::PropertyTree          &tree
					);

				/**
				* Populate the given tree with the meshes within the scene
				* @param tree tree to populate
				*/
				void populateWithMeshes(
					repo::lib::PropertyTree          &tree);

				/**
				* Populate the given tree with the asset
				* @param node node to define the asset from
				* @param tree tree to populate
				*/
				void populateWithNode(
					const repo::core::model::RepoNode* node,
					repo::lib::PropertyTree          &tree);
		
				/**
				* write buffered binary files into the tree
				* @param tree tree to write into
				*/
				void writeBuffers(
					repo::lib::PropertyTree &tree);
			};




		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo

