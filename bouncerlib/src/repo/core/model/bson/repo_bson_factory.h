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
*  BSON factory that creates specific BSON objects.
*  This essentially functions as the builder (like Mongo BsonObjBuilder)
*/

#pragma once
#include "../repo_node_utils.h"

#include "repo_bson_project_settings.h"
#include "repo_bson_user.h"
#include "repo_node.h"
#include "repo_node_camera.h"
#include "repo_node_metadata.h"
#include "repo_node_material.h"
#include "repo_node_map.h"
#include "repo_node_mesh.h"
#include "repo_node_reference.h"
#include "repo_node_revision.h"
#include "repo_node_texture.h"
#include "repo_node_transformation.h"



namespace repo {
	namespace core {
		namespace model {
				class REPO_API_EXPORT RepoBSONFactory
				{
					public:

						
						/**
						* Create a project setting BSON
						* @param uniqueProjectName a unique name for the project
						* @param owner owner ofthis project
						* @param group group with access of this project
						* @param type  type of project
						* @param description Short description of the project
						* @param ownerPermissionsOctal owner's permission in POSIX
						* @param groupPermissionsOctal group's permission in POSIX
						* @param publicPermissionsOctal other's permission in POSIX
						* @return returns a Project settings
						*/
						static RepoProjectSettings makeRepoProjectSettings(
							const std::string &uniqueProjectName,
							const std::string &owner,
							const std::string &group = std::string(),
							const std::string &type = REPO_PROJECT_TYPE_ARCHITECTURAL,
							const std::string &description = std::string(),
							const uint8_t     &ownerPermissionsOctal = 7,
							const uint8_t     &groupPermissionsOctal = 0,
							const uint8_t     &publicPermissionsOctal = 0);

						/**
						* Create a user BSON
						* @param userName username
						* @param password password of the user
						* @param firstName first name of user
						* @param lastName last name of user
						* @param email  email address of the user
						* @param projects list of projects the user has access to
						* @param roles list of roles the users are capable of
						* @param groups list of groups the user belongs to
						* @param apiKeys a list of api keys for the user
						* @param avatar picture of the user
						* @return returns a RepoUser
						*/

						static RepoUser makeRepoUser(
							const std::string                                      &userName,
							const std::string                                      &password,
							const std::string                                      &firstName,
							const std::string                                      &lastName,
							const std::string                                      &email,
							const std::list<std::pair<std::string, std::string>>   &projects,
							const std::list<std::pair<std::string, std::string>>   &roles,
							const std::list<std::pair<std::string, std::string>>   &groups,
							const std::list<std::pair<std::string, std::string>>   &apiKeys,
							const std::vector<char>                                &avatar);
							


						/*
						* -------------------- REPO NODES ------------------------
						*/

						/**
						* Append default information onto the a RepoBSONBuilder
						* This is used for children nodes to create their BSONs.
						* @param builder the builder to append the info to
						* @param type type of node
						* @param api api level of this node
						* @param shareID shared ID of this node
						* @param name name of the node
						* @param parents vector of shared IDs of this node's parents
						* @ return return the size of the fields appended in bytes
						*/
						static uint64_t appendDefaults(
							RepoBSONBuilder &builder,
							const std::string &type,
							const unsigned int api = REPO_NODE_API_LEVEL_0,
							const repoUUID &sharedId = generateUUID(),
							const std::string &name = std::string(),
							const std::vector<repoUUID> &parents = std::vector<repoUUID>());

						/**
						* Create a RepoNode
						* @param type of node (Optional)
						* @return returns a RepoNode
						*/
						static RepoNode makeRepoNode (std::string type=std::string());

						/**
						* Create a Camera Node
						* @param aspect ratio
						* @param Far clipping plane
						* @param Near clipping plane. Should not be 0 to avoid divis
						* @param Field of view.
						* @param LookAt vector relative to parent transformations.
						* @param Position relative to parent transformations
						* @param Up vector relative to parent transformations.
						* @param API level of the node (optional, default REPO_NODE_API_LEVEL_1)
						* @param name of the node (optional, default empty string)
						* @return returns a Camera node
						*/
						static CameraNode makeCameraNode(
							const float         &aspectRatio,
							const float         &farClippingPlane,
							const float         &nearClippingPlane,
							const float         &fieldOfView,
							const repo_vector_t &lookAt,
							const repo_vector_t &position,
							const repo_vector_t &up,
							const std::string   &name = std::string(),
							const int           &apiLevel = REPO_NODE_API_LEVEL_1);


						/**
						* Create a Material Node
						* @param a struct that contains the info about the material
						* @param name of the Material
						* @return returns a Material node
						*/
						static MaterialNode makeMaterialNode(
							const repo_material_t &material,
							const std::string     &name,
							const int             &apiLevel = REPO_NODE_API_LEVEL_1);

						/**
						* Create a Map Node
						* @param a struct that contains the info about the material
						* @param name of the Map
						* @return returns a Map node
						*/
						static MapNode makeMapNode(
							const uint32_t        &width,
							const uint32_t        &zoom,
							const float           &tilt,
							const float           &tileSize,
							const float           &longitude,
							const float           &latitude,
							const repo_vector_t   &centrePoint,
							const std::string     &name = std::string(),
							const int             &apiLevel = REPO_NODE_API_LEVEL_1);

						/**
						* Create a Metadata Node
						* @param metadata Metadata itself in RepoBSON format
						* @param mimtype Mime type, the media type of the metadata (optional)
						* @param name Name of Metadata (optional)
						* @param parents 
						* @param apiLevel Repo Node API level (optional)
						* @return returns a metadata node
						*/
						static MetadataNode makeMetaDataNode(
							RepoBSON			         &metadata,
							const std::string            &mimeType = std::string(),
							const std::string            &name = std::string(),
							const std::vector<repoUUID> &parents = std::vector<repoUUID>(),
							const int                    &apiLevel = REPO_NODE_API_LEVEL_1);


						/**
						* Create a Metadata Node
						* @param keys labels for the fields
						* @param values values of the fields, matching the key parameter
						* @param name Name of Metadata (optional)
						* @param parents
						* @param apiLevel Repo Node API level (optional)
						* @return returns a metadata node
						*/
						static MetadataNode makeMetaDataNode(
							const std::vector<std::string>  &keys,
							const std::vector<std::string>  &values,
							const std::string               &name = std::string(),
							const std::vector<repoUUID>     &parents = std::vector<repoUUID>(),
							const int                       &apiLevel = REPO_NODE_API_LEVEL_1);
						/**
						* Create a Mesh Node
						* @param vertices vector of vertices
						* @param faces vector of faces
						* @param normals vector of normals
						* @param boundingBox vector of 2 vertex indicating the bounding box
						* @param uvChannels vector of UV Channels
						* @param colors vector of colours
						* @param outline outline generated from bounding box
						* @param name name of the node (optional, default empty string)
						* @param apiLevel API level of the node (optional, default REPO_NODE_API_LEVEL_1)
						* @return returns a mesh node
						*/
						static MeshNode makeMeshNode(
							std::vector<repo_vector_t>                  &vertices,
							std::vector<repo_face_t>                    &faces,
							std::vector<repo_vector_t>                  &normals,
							std::vector<std::vector<float>>             &boundingBox,
							std::vector<std::vector<repo_vector2d_t>>   &uvChannels,
							std::vector<repo_color4d_t>                 &colors,
							std::vector<std::vector<float>>             &outline,
							const std::string                           &name = std::string(),
							const int                                   &apiLevel = REPO_NODE_API_LEVEL_1);

						/**
						* Create a Reference Node
						* If revision ID is unique, it will be referencing a specific revision
						* If it isn't unique, it will represent a branch ID and reference its head
						* @param database name of the database to reference
						* @param project name of the project to reference
						* @param revisionID uuid of the revision (default: master branch)
						* @param isUniqueID is revisionID a specific revision ID (default: false - i.e it is a branch ID)
						* @return returns a reference node
						*/
						static ReferenceNode makeReferenceNode(
							const std::string &database,
							const std::string &project,
							const repoUUID    &revisionID = stringToUUID(REPO_HISTORY_MASTER_BRANCH),
							const bool        &isUniqueID = false,
							const std::string &name = std::string(),
							const int         &apiLevel = REPO_NODE_API_LEVEL_1);

						/**
						* Create a Revision Node
						* @param user name of the user who is commiting this project
						* @param branch UUID of the branch
						* @param currentNodes vector of current nodes (unique IDs)
						* @param added    vector of what nodes are added    (shared IDs) 
						* @param removed  vector of what nodes are deleted  (shared IDs) 
						* @param modified vector of what nodes are modified (shared IDs) 
						* @param modified vector of what nodes are modified (shared IDs)
						* @param parent   UUID of parent (in a vector)
						* @param message  A message to describe what this commit is for (optional)
						* @param tag      A tag for this specific revision (optional)
						* @param apiLevel API Level(optional)
						* @return returns a texture node
						*/

						static RevisionNode makeRevisionNode(
							const std::string			 &user,
							const repoUUID              &branch,
							const std::vector<repoUUID> &currentNodes,
							const std::vector<repoUUID> &added,
							const std::vector<repoUUID> &removed,
							const std::vector<repoUUID> &modified,
							const std::vector<repoUUID> &parent = std::vector<repoUUID>(),
							const std::string            &message = std::string(),
							const std::string            &tag = std::string(),
							const int                    &apiLevel = REPO_NODE_API_LEVEL_1
							);


						/**
						* Create a Texture Node
						* @param name name of the texture node, with extension to indicate file format
						* @param data binary data to store
						* @param byteCount size of the binary data in bytes
						* @param width width of the texture
						* @param height height of the texture
						* @param apiLevel API Level(optional)
						* @return returns a texture node
						*/
						static TextureNode makeTextureNode(
							const std::string &name,
							const char        *data,
							const uint32_t    &byteCount,
							const uint32_t    &width,
							const uint32_t    &height,
							const int         &apiLevel = REPO_NODE_API_LEVEL_1);

						/**
						* Create a Transformation Node
						* @param transMatrix a 4 by 4 transformation matrix (optional - default is identity matrix)
						* @param name name of the transformation(optional)
						* @param parents vector of parents uuid for this node (optional)
						* @param apiLevel API Level(optional)
						* @return returns a Transformation node
						*/
						static TransformationNode makeTransformationNode(
						const std::vector<std::vector<float>> &transMatrix = TransformationNode::identityMat(),
						const std::string                     &name = "<transformation>",
						const std::vector<repoUUID>		      &parents = std::vector<repoUUID>(),
						const int                             &apiLevel = REPO_NODE_API_LEVEL_1);

				};
		} //namespace model
	} //namespace core
} //namespace repo
