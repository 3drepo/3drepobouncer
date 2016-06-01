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

#include "repo_model_export_web.h"
#include "../../../lib/repo_property_tree.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			class GLTFModelExport : public WebModelExport
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

				/**
				* Export all necessary files as buffers
				* @return returns a repo_src_export_t containing all files needed for this
				*          model to be rendered
				*/
				repo_web_buffers_t getAllFilesExportedAsBuffer() const;

			private:
				std::unordered_map<std::string, std::vector<uint8_t>> fullDataBuffer;

				void addAccessors(
					const std::string              &accName,
					const std::string              &buffViewName,
					repo::lib::PropertyTree        &tree,
					const std::vector<uint16_t>    &faces,
					const uint32_t                 &addrFrom,
					const uint32_t                 &addrTo,
					const std::string              &refId = std::string(),
					const std::vector<uint16_t>    &lod = std::vector<uint16_t>(),
					const size_t                   &offset = 0);

				void addAccessors(
					const std::string              &accName,
					const std::string              &buffViewName,
					repo::lib::PropertyTree        &tree,
					const std::vector<float>       &data,
					const uint32_t                 &addrFrom,
					const uint32_t                 &addrTo,
					const std::string              &refId,
					const size_t                   &offset);

				void addAccessors(
					const std::string                  &accName,
					const std::string                  &buffViewName,
					repo::lib::PropertyTree            &tree,
					const std::vector<repo_vector2d_t> &buffer,
					const uint32_t                     &addrFrom,
					const uint32_t                     &addrTo,
					const std::string                  &refId = std::string(),
					const size_t                       &offset = 0);

				void addAccessors(
					const std::string                &accName,
					const std::string                &buffViewName,
					repo::lib::PropertyTree          &tree,
					const std::vector<repo_vector_t> &buffer,
					const uint32_t                   &addrFrom,
					const uint32_t                   &addrTo,
					const std::string                &refId = std::string(),
					const size_t                     &offset = 0);

				/**
				* Add an accessor to a bufferview
				* @param accName name of accessor
				* @param buffViewName name of buffer
				* @param tree tree to add the properties in
				* @param count count of elements (if it's a vector of 3 floats, 1 count is 3 floats)
				* @param offset offset to the starting position of the accessor in bytes
				* @param stride stride of each element in bytes
				* @param componentType type of component this array holds
				* @param bufferType the type of buffer this array holds
				* @param min minimum value of this array
				* @param max maximum value of this array
				*/
				void addAccessors(
					const std::string              &accName,
					const std::string              &buffViewName,
					repo::lib::PropertyTree        &tree,
					const size_t                   &count,
					const size_t                   &offset,
					const size_t                   &stride,
					const uint32_t                 &componentType,
					const std::string              &bufferType,
					const std::vector<float>       &min,
					const std::vector<float>       &max,
					const std::string              &refId = std::string(),
					const std::vector<uint16_t>    &lod = std::vector<uint16_t>());

				/**
				* Add a buffer view into a buffer,
				* also provide this partial buffer to add into the
				* binary file (buffer)
				* @param name name of the buffer
				* @param fileName name of binary file
				* @param tree tree to insert header info into
				* @param buffer buffer to export
				*/
				void addBufferView(
					const std::string                   &name,
					const std::string                   &fileName,
					repo::lib::PropertyTree             &tree,
					const std::vector<uint16_t>         &buffer,
					const size_t                        &offset,
					const size_t                        &count,
					const std::string                   &refId = std::string()
					);

				void addBufferView(
					const std::string                   &name,
					const std::string                   &fileName,
					repo::lib::PropertyTree             &tree,
					const std::vector<float>            &buffer,
					const size_t                        &offset,
					const size_t                        &count,
					const std::string                   &refId);

				void addBufferView(
					const std::string                   &name,
					const std::string                   &fileName,
					repo::lib::PropertyTree             &tree,
					const std::vector<repo_vector_t>    &buffer,
					const size_t                        &offset,
					const size_t                        &count,
					const std::string                   &refId = std::string()
					);

				void addBufferView(
					const std::string                   &name,
					const std::string                   &fileName,
					repo::lib::PropertyTree             &tree,
					const std::vector<repo_vector2d_t>  &buffer,
					const size_t                        &offset,
					const size_t                        &count,
					const std::string                   &refId = std::string()
					);

				/**
				* Add a buffer view into a buffer,
				* also provide this partial buffer to add into the
				* binary file (buffer)
				* @param name name of the buffer
				* @param fileName name of binary file
				* @param tree tree to insert header info into
				* @param buffer buffer to export
				*/
				void addBufferView(
					const std::string                   &name,
					const std::string                   &fileName,
					repo::lib::PropertyTree             &tree,
					const size_t						&byteLength,
					const size_t                        &offset,
					const uint32_t						&bufferTarget,
					const std::string                   &refId = std::string()
					);

				/**
				* Add a buffer into the given data buffer
				* @param bufferName name of the buffer to add to
				* @param the buffer to append at the back
				* @param byteLength length of the buffer to append
				* @return returns the offset where the added buffer starts
				*/
				size_t addToDataBuffer(
					const std::string              &bufferName,
					const uint8_t                  *buffer,
					const size_t                   &byteLength
					);

				template <typename T>
				size_t addToDataBuffer(
					const std::string              &bufferName,
					const std::vector<T>           &buffer
					)
				{
					return addToDataBuffer(bufferName, (uint8_t*)buffer.data(), buffer.size() * sizeof(T));
				}

				/**
				* Construct JSON document about the scene
				* @param tree tree to place the info
				* @return returns true upon success
				*/
				bool constructScene(
					repo::lib::PropertyTree &tree);

				/**
				* Create a tree representation for the graph
				* This creates the header of the SRC
				* @return returns true upon success
				*/
				bool generateTreeRepresentation();

				/**
				* Generate a property tree representing the spatial partitioning
				* @return return a property tree with adequate entries
				*/
				repo::lib::PropertyTree generateSpatialrepo_partitioning_tree_t();

				/**
				* Return the GLTF file as raw bytes buffer
				* returns an empty vector if the export has failed
				*/
				std::unordered_map<std::string, std::vector<uint8_t>> getGLTFFilesAsBuffer() const;

				/**
				* Reindex the given faces base on the given information
				* @param matMap materialMapping as reference
				* @param faces the faces to remap
				*/
				void reIndexFaces(
					const std::vector<std::vector<repo_mesh_mapping_t>> &matMap,
					std::vector<uint16_t>                               &faces);

				/**
				* Process children of nodes(Transformation)
				* Recurse call of populateWithNode() if there is a transformation as child
				* and also property lists (meshes/cameras)
				* @param node node in question
				*/
				void processNodeChildren(
					const repo::core::model::RepoNode *node,
					repo::lib::PropertyTree          &tree,
					const std::unordered_map<repoUUID, uint32_t, RepoUUIDHasher> &subMeshCounts
					);

				/**
				* Populate the given tree with the cameras within the scene
				* @param tree tree to populate
				*/
				void populateWithCameras(
					repo::lib::PropertyTree          &tree);

				/**
				* Populate the given tree with the materials within the scene
				* @param tree tree to populate
				*/
				void populateWithMaterials(
					repo::lib::PropertyTree          &tree);

				/**
				* @param tree tree to populate
				*/
				std::unordered_map<repoUUID, uint32_t, RepoUUIDHasher>
					populateWithMeshes(
					repo::lib::PropertyTree           &tree);

				/**
				* Populate the given tree with the textures within the scene
				* @param tree tree to populate
				*/
				void populateWithTextures(
					repo::lib::PropertyTree          &tree);

				/**
				* Populate the given tree with transformations
				* @param tree tree to populate
				*/
				void populateWithNodes(
					repo::lib::PropertyTree          &tree,
					const std::unordered_map<repoUUID, uint32_t, RepoUUIDHasher> &subMeshCounts);

				std::vector<std::vector<std::vector<uint16_t>>> reorderFaces(
					std::vector<uint16_t>                               &faces,
					const std::vector<repo_vector_t>                    &vertices,
					const std::vector<std::vector<repo_mesh_mapping_t>> &mapping);

				/**
				* Reorder a certain chunk of faces base on quantization
				* @param faces faces array to reOrder
				* @param vertices reference vertices
				* @param mapping mapping detailing which chunk of face to reorder
				* @return returns the reordered version of the faces
				*/
				std::vector<uint16_t> reorderFaces(
					const std::vector<uint16_t>      &faces,
					const std::vector<repo_vector_t> &vertices,
					const repo_mesh_mapping_t        &mapping,
					std::vector<uint16_t>      &lods) const;

				std::vector<uint16_t> serialiseFaces(
					const std::vector<repo_face_t> &faces) const;

				/**
				* write buffered binary files into the tree
				* @param tree tree to write into
				*/
				void writeBuffers(
					repo::lib::PropertyTree &tree);

				/**
				* Write the default sampler into the
				* property tree. Unless specified, all textures
				* will be using this sampler
				* Note: there is currently no way to specify
				* your own specific sampler
				* @param tree tree to write into
				*/
				void writeDefaultSampler(
					repo::lib::PropertyTree &tree
					);

				/**
				* Write the default shading technique into the
				* property tree. Unless specified, all materials
				* will be rendered using this technique
				* Note: there is currently no way to specify
				* your own specific technique/shader
				* @param tree tree to write into
				*/
				void writeDefaultTechnique(
					repo::lib::PropertyTree &tree
					);
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
