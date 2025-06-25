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
#include "repo/repo_bouncer_global.h"
#include "repo/lib/datastructure/repo_structs.h"
#include "repo/lib/datastructure/repo_bounds.h"

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
#define REPO_NODE_MESH_LABEL_BOUNDING_BOX			"bounding_box" //!< bounding box
			//------------------------------------------------------------------------------
#define REPO_NODE_MESH_LABEL_UV_CHANNELS				"uv_channels" //!< uv channels array
#define REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT		"uv_channels_count"
#define REPO_NODE_MESH_LABEL_UV_CHANNELS_BYTE_COUNT	"uv_channels_byte_count"
#define REPO_NODE_MESH_LABEL_SHA256                  "sha256"
#define REPO_NODE_MESH_LABEL_COLORS                  "colors"
#define REPO_NODE_MESH_LABEL_SUBMESH_IDS                     "submeshIds"
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

			class REPO_API_EXPORT MeshNode : public RepoNode
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
				MeshNode(RepoBSON bson);

				/**
				* Default deconstructor
				*/
				~MeshNode();

			protected:
				virtual void deserialise(RepoBSON&);
				virtual void serialise(repo::core::model::RepoBSONBuilder&) const;

			public:

				/**
				* Returns a number, indicating the mesh's format
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

			protected:
				MeshNode::Primitive primitive;
				repo::lib::RepoBounds boundingBox;
				std::vector<repo::lib::repo_face_t> faces;
				std::vector<repo::lib::RepoVector3D> vertices;
				std::vector<repo::lib::RepoVector3D> normals;
				std::vector<std::vector<repo::lib::RepoVector2D>> channels;

				// Filters for supermeshing
				std::string grouping;
				bool isOpaque = false;
				bool isTransparent = false;
				repo::lib::RepoUUID textureId;

				// Material struct
				repo::lib::repo_material_t material;

			public:
				/**
				* Get the mesh primitive type (points, lines, triangles, quads) (triangles if not set).
				*/
				virtual MeshNode::Primitive getPrimitive() const
				{
					return primitive;
				}

				/**
				* Check if the node is position dependant.
				* i.e. if parent transformation is merged onto the node,
				* does the node requre to a transformation applied to it
				* e.g. meshes and cameras are position dependant, metadata isn't
				* Default behaviour is false. Position dependant child requires
				* override this function.
				* @return true if node is positionDependant.
				*/
				virtual bool positionDependant()
				{
					return true;
				}

				/**
				* Check if the node is semantically equal to another
				* Different node should have a different interpretation of what
				* this means.
				* @param other node to compare with
				* @param returns true if equal, false otherwise
				*/
				virtual bool sEqual(const RepoNode &other) const;

				/**
				*  Create a new object with transformation applied to the node
				* default behaviour is do nothing. Children object
				* needs to override this function to perform their own specific behaviour.
				* @param matrix transformation matrix to apply.
				* @return returns a new object with transformation applied.
				*/
				MeshNode cloneAndApplyTransformation(
					const repo::lib::RepoMatrix &matrix) const;

				void applyTransformation(
					const repo::lib::RepoMatrix& matrix);

				void setGrouping(const std::string& grouping)
				{
					this->grouping = grouping;
				}

				void setMaterial(const repo::lib::repo_material_t& m) {
					this->material = m;

					// Set filter flags from material properties
					this->isOpaque = (m.opacity == 1.f);
					this->isTransparent = !(this->isOpaque);
				}

				const repo::lib::repo_material_t& getMaterial() {
					return material;
				}

				void setTextureId(const repo::lib::RepoUUID textureId) {
					this->textureId = textureId;
				}

				/**
				* --------- Convenience functions -----------
				*/

				/**
				* Retrieve the bounding box of this mesh
				* @return returns a vector of size 2, containing the bounding box.
				*/
				repo::lib::RepoBounds getBoundingBox() const
				{
					return boundingBox;
				}

				void setBoundingBox(const repo::lib::RepoBounds& bounds)
				{
					boundingBox = bounds;
				}

				/**
				* Retrieve a vector of faces from the bson object
				*/
				std::vector<repo::lib::repo_face_t> getFaces() const
				{
					return faces;
				}

				void setFaces(const std::vector<repo::lib::repo_face_t>& faces)
				{
					this->faces = std::vector<repo::lib::repo_face_t>(faces.begin(), faces.end());
					if (this->faces.size()) {
						primitive = (Primitive)this->faces[0].size();
					}
				}

				// get sepcific grouping for mesh batching (empty string if not specified)
				std::string getGrouping() const
				{
					return grouping;
				}

				/**
				* Retrieve a vector of vertices from the bson object
				*/
				const std::vector<repo::lib::RepoVector3D>& getNormals() const
				{
					return normals;
				}

				void setNormals(const std::vector<repo::lib::RepoVector3D>& normals)
				{
					this->normals = normals;
				}

				std::vector<repo::lib::RepoVector2D> getUVChannelsSerialised() const;

				/**
				* Retrieve a vector of UV Channels, separated by channels
				*/
				std::vector<std::vector<repo::lib::RepoVector2D>> getUVChannelsSeparated() const
				{
					return channels;
				}

				void setUVChannel(size_t channel, std::vector<repo::lib::RepoVector2D> uvs);

				/**
				* Retrieve a vector of vertices from the bson object
				*/
				const std::vector<repo::lib::RepoVector3D>& getVertices() const
				{
					return vertices;
				}

				void setVertices(const std::vector<repo::lib::RepoVector3D>& vertices, bool updateBoundingBox = false)
				{
					this->vertices = std::vector<repo::lib::RepoVector3D>(vertices.begin(), vertices.end());
					if (updateBoundingBox) {
						this->updateBoundingBox();
					}
				}

				std::uint32_t getNumFaces() const
				{
					return faces.size();
				}

				std::uint32_t getNumVertices() const
				{
					return vertices.size();
				}

				std::uint32_t getNumUVChannels() const
				{
					return channels.size();
				}

				size_t getSize() const;

				void updateBoundingBox();

				/*
				* Compresses the mesh by rebuilding the vertices array, redirecting indices
				* to existing vertices where possible and removing the duplicates.
				*/
				void removeDuplicateVertices();

				static void transformBoundingBox(repo::lib::RepoBounds& bounds, repo::lib::RepoMatrix matrix);
			};
		} //namespace model
	} //namespace core
} //namespace repo
