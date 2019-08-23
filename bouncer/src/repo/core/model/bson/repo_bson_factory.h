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

#include "repo_bson_project_settings.h"
#include "repo_bson_ref.h"
#include "repo_bson_role.h"
#include "repo_bson_role_settings.h"
#include "repo_bson_user.h"
#include "repo_bson_unity_assets.h"
#include "repo_node.h"
#include "repo_node_camera.h"
#include "repo_node_metadata.h"
#include "repo_node_material.h"
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
				* @param isFederate is the project a federation
				* @param group group with access of this project
				* @param type  type of project
				* @param description Short description of the project
				* @return returns a Project settings
				*/
				static RepoProjectSettings makeRepoProjectSettings(
					const std::string &uniqueProjectName,
					const std::string &owner,
					const bool        &isFederate,
					const std::string &type = REPO_DEFAULT_PROJECT_TYPE_ARCHITECTURAL,
					const std::string &description = std::string(),
					const double pinSize = REPO_DEFAULT_PROJECT_PIN_SIZE,
					const double avatarHeight = REPO_DEFAULT_PROJECT_AVATAR_HEIGHT,
					const double visibilityLimit = REPO_DEFAULT_PROJECT_VISIBILITY_LIMIT,
					const double speed = REPO_DEFAULT_PROJECT_SPEED,
					const double zNear = REPO_DEFAULT_PROJECT_ZNEAR,
					const double zFar = REPO_DEFAULT_PROJECT_ZFAR);

				/**
				* Create a role BSON from previous role
				* Use _makeRepoRole() if you wish to have direct control on the interface
				* @param roleName name of the role
				* @param database database where this role resides
				* @param permissions a vector of project and their access permissions
				* @param oldRole previous role from which to copy over privileges and inherited roles
				* @return returns a bson with this role information
				*/
				static RepoRole makeRepoRole(
					const std::string &roleName,
					const std::string &database,
					const std::vector<RepoPermission> &permissions = std::vector<RepoPermission>(),
					const RepoRole &oldRole = RepoRole());

				/**
				* Create RepoRef
				* @param fileName name of the file
				* @param type type of storage
				* @param link reference link
				* @param size size of file in bytes
				* @return returns a bson with this reference information
				*/
				static RepoRef makeRepoRef(
					const std::string &fileName,
					const RepoRef::RefType &type,
					const std::string &link,
					const uint32_t size);

				/**
				* Create a role BSON
				* @param roleName name of the role
				* @param database database where this role resides
				* @param privileges a vector of privileges this role has
				* @param inhertedRoles vector of roles which this role inherits from
				* @return returns a bson with this role information
				*/
				static RepoRole _makeRepoRole(
					const std::string &roleName,
					const std::string &database,
					const std::vector<RepoPrivilege> &privileges,
					const std::vector<std::pair<std::string, std::string>> &inheritedRoles
					= std::vector<std::pair<std::string, std::string>>()
					);

				static RepoRoleSettings makeRepoRoleSettings(
					const std::string &uniqueRoleName,
					const std::string &color,
					const std::string &description = std::string(),
					const std::vector<std::string> &modules = std::vector<std::string>());

				/**
				* Create a user BSON
				* @param userName username
				* @param password password of the user
				* @param firstName first name of user
				* @param lastName last name of user
				* @param email  email address of the user
				* @param roles list of roles the users are capable of
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
					const std::list<std::pair<std::string, std::string>>   &roles,
					const std::list<std::pair<std::string, std::string>>   &apiKeys,
					const std::vector<char>                                &avatar);

				/**
				* Create a Unity assets list BSON
				* @param revisionID uuid of the revision (default: master branch)
				* @param assets list of Unity assets
				* @param database name of the database to reference
				* @param model model ID (string) to reference
				* @param offset world offset shift coordinates of the model
				* @param vrAssetFiles list of VR Unity assets
				* @param iosAssetFiles list of iOS Unity assets
				* @param jsonFiles list of JSON files
				* @return returns a RepoUnityAssets
				*/
				static RepoUnityAssets makeRepoUnityAssets(
					const repo::lib::RepoUUID                              &revisionID,
					const std::vector<std::string>                         &assets,
					const std::string                                      &database,
					const std::string                                      &model,
					const std::vector<double>                              &offset,
					const std::vector<std::string>                         &vrAssetFiles,
					const std::vector<std::string>                         &iosAssetFiles,
					const std::vector<std::string>                         &jsonFiles);

				/*
				* -------------------- REPO NODES ------------------------
				*/

				/**
				* Append default information onto the a RepoBSONBuilder
				* This is used for children nodes to create their BSONs.
				* @param type type of node
				* @param api api level of this node
				* @param shareID shared ID of this node
				* @param name name of the node
				* @param parents vector of shared IDs of this node's parents
				* @param uniqueID specify unique ID for the object (do not use unless you are
				*			sure you know what you're doing!)
				* @ return return a bson object with the default parameters
				*/
				static RepoBSON appendDefaults(
					const std::string &type,
					const unsigned int api = REPO_NODE_API_LEVEL_0,
					const repo::lib::RepoUUID &sharedId = repo::lib::RepoUUID::createUUID(),
					const std::string &name = std::string(),
					const std::vector<repo::lib::RepoUUID> &parents = std::vector<repo::lib::RepoUUID>(),
					const repo::lib::RepoUUID &uniqueID = repo::lib::RepoUUID::createUUID());

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
					const repo::lib::RepoVector3D &lookAt,
					const repo::lib::RepoVector3D &position,
					const repo::lib::RepoVector3D &up,
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
					const std::vector<repo::lib::RepoUUID> &parents = std::vector<repo::lib::RepoUUID>(),
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
					const std::vector<repo::lib::RepoUUID>     &parents = std::vector<repo::lib::RepoUUID>(),
					const int                       &apiLevel = REPO_NODE_API_LEVEL_1);

				/**
				* Create a Metadata Node
				* @param data list of key value pair
				* @param name Name of Metadata (optional)
				* @param parents
				* @param apiLevel Repo Node API level (optional)
				* @return returns a metadata node
				*/
				static MetadataNode makeMetaDataNode(
					const std::unordered_map<std::string, std::string>  &data,
					const std::string            &name = std::string(),
					const std::vector<repo::lib::RepoUUID> &parents = std::vector<repo::lib::RepoUUID>(),
					const int                    &apiLevel = REPO_NODE_API_LEVEL_1);
				

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
					const std::vector<repo::lib::RepoVector3D>                  &vertices,
					const std::vector<repo_face_t>                    &faces,
					const std::vector<repo::lib::RepoVector3D>                  &normals = std::vector<repo::lib::RepoVector3D>(),
					const std::vector<std::vector<float>>             &boundingBox = std::vector<std::vector<float>>(),
					const std::vector<std::vector<repo::lib::RepoVector2D>>   &uvChannels = std::vector<std::vector<repo::lib::RepoVector2D>>(),
					const std::vector<repo_color4d_t>                 &colors = std::vector<repo_color4d_t>(),
					const std::vector<std::vector<float>>             &outline = std::vector<std::vector<float>>(),
					const std::string                                 &name = std::string(),
					const std::vector<repo::lib::RepoUUID>            &parents = std::vector<repo::lib::RepoUUID>(),
					const int                                         &apiLevel = REPO_NODE_API_LEVEL_1);

				static MeshNode makeMeshNode(
					const std::vector<repo::lib::RepoVector3D>        &vertices,
					const std::vector<repo_face_t>                    &faces,
					const std::vector<repo::lib::RepoVector3D>        &normals,
					const std::vector<std::vector<float>>             &boundingBox,
					const std::vector<repo::lib::RepoUUID>            &parents) {

					return makeMeshNode(vertices, faces, normals, boundingBox,
						std::vector<std::vector<repo::lib::RepoVector2D>>(),
						std::vector<repo_color4d_t>(),
						std::vector<std::vector<float>>(),
						std::string(),
						parents
					);
				}

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
					const repo::lib::RepoUUID    &revisionID = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH),
					const bool        &isUniqueID = false,
					const std::string &name = std::string(),
					const int         &apiLevel = REPO_NODE_API_LEVEL_1);

				/**
				* Create a Revision Node
				* @param user name of the user who is commiting this project
				* @param branch UUID of the branch
				* @param currentNodes vector of current nodes (unique IDs)
				//* @param added    vector of what nodes are added    (shared IDs)
				//* @param removed  vector of what nodes are deleted  (shared IDs)
				//* @param modified vector of what nodes are modified (shared IDs)
				* @param files    vector of the original files for this model
				* @param parent   UUID of parent (in a vector)
				* @param worldOffset x,y,z offset for world coordinates
				* @param message  A message to describe what this commit is for (optional)
				* @param tag      A tag for this specific revision (optional)
				* @param apiLevel API Level(optional)
				* @return returns a texture node
				*/

				static RevisionNode makeRevisionNode(
					const std::string			   &user,
					const repo::lib::RepoUUID                 &branch,
					const std::vector<repo::lib::RepoUUID>    &currentNodes,
					//const std::vector<repo::lib::RepoUUID>    &added,
					//const std::vector<repo::lib::RepoUUID>    &removed,
					//const std::vector<repo::lib::RepoUUID>    &modified,
					const std::vector<std::string> &files = std::vector<std::string>(),
					const std::vector<repo::lib::RepoUUID>    &parent = std::vector<repo::lib::RepoUUID>(),
					const std::vector<double>       &worldOffset = std::vector<double>(),
					const std::string              &message = std::string(),
					const std::string              &tag = std::string(),
					const int                      &apiLevel = REPO_NODE_API_LEVEL_1
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
					const repo::lib::RepoMatrix &transMatrix = repo::lib::RepoMatrix(),
					const std::string                     &name = "<transformation>",
					const std::vector<repo::lib::RepoUUID>		      &parents = std::vector<repo::lib::RepoUUID>(),
					const int                             &apiLevel = REPO_NODE_API_LEVEL_1);
			};
		} //namespace model
	} //namespace core
} //namespace repo
