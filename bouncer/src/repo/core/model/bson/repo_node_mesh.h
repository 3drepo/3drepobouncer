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
* Mesh Node
*/

#pragma once
#include "repo_node.h"

#include "../../../repo_bouncer_global.h"
#include "../../../lib/datastructure/repo_structs.h"

namespace repo {
	namespace core {
		namespace model {
			//------------------------------------------------------------------------------
			//
			// Fields specific only to mesh
			//
			//------------------------------------------------------------------------------
#define REPO_NODE_MESH_LABEL_VERTICES				"vertices" //<! vertices array
#define REPO_NODE_MESH_LABEL_VERTICES_COUNT			"vertices_count" //<! vertices size
#define REPO_NODE_MESH_LABEL_VERTICES_BYTE_COUNT		"vertices_byte_count"
			//------------------------------------------------------------------------------
#define REPO_NODE_MESH_LABEL_FACES					"faces" //<! faces array label
#define REPO_NODE_MESH_LABEL_FACES_COUNT				"faces_count" //<! number of faces
#define REPO_NODE_MESH_LABEL_FACES_BYTE_COUNT		"faces_byte_count"
			//------------------------------------------------------------------------------
#define REPO_NODE_MESH_LABEL_NORMALS					"normals" //!< normals array label
			//------------------------------------------------------------------------------
#define REPO_NODE_MESH_LABEL_OUTLINE					"outline" //!< outline array label
#define REPO_NODE_MESH_LABEL_BOUNDING_BOX			"bounding_box" //!< bounding box
			//------------------------------------------------------------------------------
#define REPO_NODE_MESH_LABEL_UV_CHANNELS				"uv_channels" //!< uv channels array
#define REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT		"uv_channels_count"
#define REPO_NODE_MESH_LABEL_UV_CHANNELS_BYTE_COUNT	"uv_channels_byte_count"
#define REPO_NODE_MESH_LABEL_SHA256                  "sha256"
#define REPO_NODE_MESH_LABEL_COLORS                  "colors"
			//------------------------------------------------------------------------------
#define REPO_NODE_MESH_LABEL_MAP_ID			        "map_id"
#define REPO_NODE_MESH_LABEL_MAP_SHARED_ID			"shared_id"
#define REPO_NODE_MESH_LABEL_VERTEX_FROM 		    "v_from"
#define REPO_NODE_MESH_LABEL_VERTEX_TO 		        "v_to"
#define REPO_NODE_MESH_LABEL_TRIANGLE_FROM	        "t_from"
#define REPO_NODE_MESH_LABEL_TRIANGLE_TO		    "t_to"
#define REPO_NODE_MESH_LABEL_MATERIAL_ID		    "mat_id"
#define REPO_NODE_MESH_LABEL_MERGE_MAP		        "m_map"
			//------------------------------------------------------------------------------
#define REPO_NODE_MESH_LABEL_GROUPING		        "grouping"
			//------------------------------------------------------------------------------
#define REPO_NODE_MESH_LABEL_PRIMITIVE		        "primitive"

			class REPO_API_EXPORT MeshNode :public RepoNode
			{
			public:
				enum class Primitive {
					UNKNOWN = 0,
					POINTS = 1,
					LINES = 2,
					TRIANGLES = 3,
					QUADS = 4
				};

				/**
				* Default constructor
				*/
				MeshNode();

				/**
				* Construct a MeshNode from a RepoBSON object
				* @param RepoBSON object
				* @param binMapping binary mapping of fields that are too big to fit within the bson
				*/
				MeshNode(RepoBSON bson,
					const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping =
					std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>>()
				);

				/**
				* Default deconstructor
				*/
				~MeshNode();

				/**
				* Returns a number, indicating it's mesh format
				* maximum of 32 bit, each bit represent the presents of the following
				*  vertices faces normals colors #uvs #primitive
				* where vertices is the LSB
				* @return returns the mFormat flag
				*/
				uint32_t getMFormat(const bool isTransparent = false, const bool isInvisibleDefault = false) const;

				/**
				* Get the type of node
				* @return returns the type as a string
				*/
				virtual std::string getType() const
				{
					return REPO_NODE_TYPE_MESH;
				}

				/**
				* Get the type of node as an enum
				* @return returns type as enum.
				*/
				virtual NodeType getTypeAsEnum() const
				{
					return NodeType::MESH;
				}

				/**
				* Get the mesh primitive type (points, lines, triangles, quads) (triangles if not set).
				*/
				virtual MeshNode::Primitive getPrimitive() const;

				/**
				* Check if the node is position dependant.
				* i.e. if parent transformation is merged onto the node,
				* does the node requre to a transformation applied to it
				* e.g. meshes and cameras are position dependant, metadata isn't
				* Default behaviour is false. Position dependant child requires
				* override this function.
				* @return true if node is positionDependant.
				*/
				virtual bool positionDependant() { return true; }

				/**
				* Check if the node is semantically equal to another
				* Different node should have a different interpretation of what
				* this means.
				* @param other node to compare with
				* @param returns true if equal, false otherwise
				*/
				virtual bool sEqual(const RepoNode &other) const;

				/*
				*	------------- Delusional modifiers --------------
				*   These are like "setters" but not. We are actually
				*   creating a new bson object with the changed field
				*/

				/**
				*  Create a new object with transformation applied to the node
				* default behaviour is do nothing. Children object
				* needs to override this function to perform their own specific behaviour.
				* @param matrix transformation matrix to apply.
				* @return returns a new object with transformation applied.
				*/
				virtual RepoNode cloneAndApplyTransformation(
					const repo::lib::RepoMatrix &matrix) const;

				/**
				* Create a new copy of the node and update its mesh mapping
				* @return returns a new meshNode with the new mappings
				*/
				MeshNode cloneAndUpdateMeshMapping(
					const std::vector<repo_mesh_mapping_t> &vec,
					const bool& overwrite = false);

				MeshNode cloneAndNoteGrouping(const std::string &group) const;

				/**
				* --------- Convenience functions -----------
				*/

				/**
				* Retrieve the bounding box of this mesh
				* @return returns a vector of size 2, containing the bounding box.
				*/
				std::vector<repo::lib::RepoVector3D> getBoundingBox() const;

				static std::vector<repo::lib::RepoVector3D> getBoundingBox(RepoBSON &bbArr);

				/**
				* Retrieve a vector of Colors from the bson object
				*/
				std::vector<repo_color4d_t> getColors() const;

				/**
				* Retrieve a vector of faces from the bson object
				*/
				std::vector<repo_face_t> getFaces() const;

				// get sepcific grouping for mesh batching (empty string if not specified)
				std::string getGrouping() const;

				std::vector<repo_mesh_mapping_t> getMeshMapping() const;

				/**
				* Retrieve a vector of vertices from the bson object
				*/
				std::vector<repo::lib::RepoVector3D> getNormals() const;

				/**
				* Retrieve a vector of UV Channels from the bson object
				*/
				std::vector<repo::lib::RepoVector2D> getUVChannels() const;

				/**
				* Retrieve a vector of UV Channels, separated by channels
				*/
				std::vector<std::vector<repo::lib::RepoVector2D>> getUVChannelsSeparated() const;

				/**
				* Retrieve a vector of vertices from the bson object
				*/
				std::vector<repo::lib::RepoVector3D> getVertices() const;

			private:
				/**
				* Given a mesh mapping, convert it into a bson object
				* @param mapping the mapping to convert
				* @return return a bson object containing the mapping
				*/
				RepoBSON meshMappingAsBSON(const repo_mesh_mapping_t  &mapping);

				/**
				* Retrieve a vector of faces (serialised) from the bson object
				*/
				std::vector<uint32_t> getFacesSerialized() const;
			};
		} //namespace model
	} //namespace core
} //namespace repo
