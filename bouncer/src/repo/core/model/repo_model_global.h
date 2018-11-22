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
* repo_model_global.h
*
*  Created on: 29 Jun 2015
*      Author: Carmen
*
*  global information for the model
*/

// http://qt-project.org/doc/qt-5/sharedlibrary.html

#pragma once

#pragma warning( disable : 4996 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 4251 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4005 )

//------------------------------------------------------------------------------
// Constants
#define REPO_ADMIN                  "admin"             //!< Admin database
#define REPO_SYSTEM_USERS           "system.users"      //!< Users collection
#define REPO_SYSTEM_ROLES           "system.roles"      //!< Roles collection

#define REPO_LABEL_ID               "_id"
#define REPO_LABEL_OVERSIZED_FILES  "_extRef"
#define REPO_LABEL_AVATAR           "avatar"
#define REPO_LABEL_DATA             "data"
#define REPO_LABEL_DATABASE         "database"
#define REPO_LABEL_HEIGHT           "height"
#define REPO_LABEL_MEDIA_TYPE       "mime"              //!< Media Type (mime type)
#define REPO_LABEL_MODEL            "model"
#define REPO_LABEL_ROLE             "role"
#define REPO_LABEL_ROLES            "roles"
#define REPO_LABEL_USER             "user"
#define REPO_LABEL_WIDTH            "width"
#define REPO_LABEL_TYPE             "type"
#define REPO_LABEL_PIN_SIZE         "pinSize"
#define REPO_LABEL_AVATAR_HEIGHT    "avatarHeight"
#define REPO_LABEL_VISIBILITY_LIMIT "visibilityLimit"
#define REPO_LABEL_SPEED            "speed"
#define REPO_LABEL_ZFAR             "zFar"
#define REPO_LABEL_ZNEAR            "zNear"

#define REPO_LABEL_DESCRIPTION      "desc"
#define REPO_LABEL_OWNER            "owner"
#define REPO_LABEL_GROUP            "group"
#define REPO_LABEL_PROPERTIES       "properties"
#define REPO_LABEL_USERS            "users"

#define REPO_LABEL_COLOR            "color"
#define REPO_LABEL_MODULES          "modules"

// Vertex/triangle map propeties
#define REPO_LABEL_MERGED_NODES     "merged_nodes"

#define REPO_COMMAND_UPDATE         "update"
#define REPO_COMMAND_UPDATES        "updates"
#define REPO_COMMAND_DELETE         "delete"
#define REPO_COMMAND_DELETES        "deletes"
#define REPO_COMMAND_UPSERT         "upsert"
#define REPO_COMMAND_LIMIT          "limit"
#define REPO_COMMAND_Q              "q"
#define REPO_COMMAND_U              "u"

#define REPO_COLLECTION_HISTORY     "history"
#define REPO_COLLECTION_ISSUES      "issues"
#define REPO_COLLECTION_RAW         "history"
#define REPO_COLLECTION_SCENE       "scene"
#define REPO_COLLECTION_STASH_REPO  "stash.3drepo"
#define REPO_COLLECTION_STASH_SRC   "stash.src"
#define REPO_COLLECTION_STASH_GLTF  "stash.gltf"
#define REPO_COLLECTION_STASH_X3D   "stash.x3d"
#define REPO_COLLECTION_STASH_JSON  "stash.json_mpc"
#define REPO_COLLECTION_STASH_UNITY "stash.unity3d"
#define REPO_COLLECTION_EXT_REF     "ref"

#define REPO_COLLECTION_SETTINGS            "settings"

//!!!!!!!!!!!!!!!!!!
// FIXME: change to settings.projects once the 3drepo.io is ready!
#define REPO_COLLECTION_SETTINGS_PROJECTS   "settings"
//!!!!!!!!!!!!!!!!!!

#define REPO_COLLECTION_SETTINGS_ROLES      "settings.roles"

// Project settings
#define REPO_DEFAULT_PROJECT_TYPE_ARCHITECTURAL "architectural"
#define REPO_DEFAULT_PROJECT_PIN_SIZE           1.6
#define REPO_DEFAULT_PROJECT_AVATAR_HEIGHT      1.6
#define REPO_DEFAULT_PROJECT_VISIBILITY_LIMIT   1000.0
#define REPO_DEFAULT_PROJECT_SPEED              5.0
#define REPO_DEFAULT_PROJECT_ZFAR               1000.0
#define REPO_DEFAULT_PROJECT_ZNEAR              0.001

#define REPO_DOCUMENT_ID_SUFFIX_FULLTREE        "fulltree.json"
#define REPO_DOCUMENT_ID_SUFFIX_IDMAP           "idMap.json"
#define REPO_DOCUMENT_ID_SUFFIX_IDTOMESHES      "idToMeshes.json"
#define REPO_DOCUMENT_ID_SUFFIX_TREEPATH        "tree_path.json"
#define REPO_DOCUMENT_ID_SUFFIX_UNITYASSETS     "unityAssets.json"
#define REPO_DOCUMENT_ID_SUFFIX_UNITY_JSON      "_unity.json.mpc"
#define REPO_DOCUMENT_ID_SUFFIX_UNITY3D         ".unity3d"
#define REPO_DOCUMENT_ID_SUFFIX_UNITY3D_WIN     "_win64.unity3d"

#define REPO_HISTORY_MASTER_BRANCH  "00000000-0000-0000-0000-000000000000"

//------------------------------------------------------------------------------
// Media Types a.k.a. as Multipurpose Internet Mail Extensions (MIME) Types
// http://www.iana.org/assignments/media-types/media-types.xhtml#image
// http://doc.qt.io/qt-5/qimagereader.html

#define REPO_MEDIA_TYPE_BMP     "image/bmp"                 //!< Windows Bitmap
#define REPO_MEDIA_TYPE_GIF     "image/gif"                 //!< Graphic Interchange Format
#define REPO_MEDIA_TYPE_JPG     "image/jpeg"                //!< Joint Photographic Experts Group
#define REPO_MEDIA_TYPE_PNG     "image/png"                 //!< Portable Network Graphics
#define REPO_MEDIA_TYPE_PBM     "image/x-portable-bitmap"   //!< Portable Bitmap
#define REPO_MEDIA_TYPE_PGM     "image/x-portable-graymap"  //!< Portable Graymap
#define REPO_MEDIA_TYPE_PPM     "image/x-portable-pixmap"   //!< Portable Pixmap
#define REPO_MEDIA_TYPE_XBM     "image/x-xbitmap"           //!< X11 Bitmap
#define REPO_MEDIA_TYPE_XPM     "image/x-xpixmap"           //!< X11 Pixmap
#define REPO_MEDIA_TYPE_SVG     "image/svg+xml"             //!< Scalable Vector Graphics

#define REPO_MEDIA_TYPE_PDF     "application/pdf"           //!< Portable Document Format
#define REPO_MEDIA_TYPE_JSON    "application/json"          //!< JavaScript Object Notation

//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Required fields
//
//-----------------------------------------------------------------------------
#define REPO_NODE_LABEL_ID				"_id"		//!< required
#define REPO_NODE_LABEL_SHARED_ID		"shared_id"	//!< required
#define REPO_NODE_LABEL_TYPE			"type"		//!< required
#define REPO_NODE_LABEL_API				"api"		//!< required
#define REPO_NODE_LABEL_PATHS			"paths"		//!< required
//-----------------------------------------------------------------------------
#define REPO_NODE_API_LEVEL_0			0 //!< unknown api level
#define REPO_NODE_API_LEVEL_1			1 //!< original api level
#define REPO_NODE_API_LEVEL_2			2 //!< triangles only api level
#define REPO_NODE_API_LEVEL_3			3 //!< compressed api level

//-----------------------------------------------------------------------------
//
// Optional nevertheless common fields
//
//-----------------------------------------------------------------------------
#define REPO_NODE_LABEL_NAME			"name" //!< optional bson field label
#define REPO_NODE_LABEL_PARENTS			"parents" //!< optional field label
#define REPO_NODE_STASH_REF              "rev_id"
//-----------------------------------------------------------------------------

#define REPO_NODE_TYPE_ANIMATION		"animation"
#define REPO_NODE_TYPE_BONE				"bone"
#define REPO_NODE_TYPE_CAMERA			"camera"
#define REPO_NODE_TYPE_COMMENT			"comment"
#define REPO_NODE_TYPE_LIGHT			"light"
#define REPO_NODE_TYPE_LOCK				"lock"
#define REPO_NODE_TYPE_MATERIAL			"material"
#define REPO_NODE_TYPE_MESH          	"mesh"
#define REPO_NODE_TYPE_METADATA     	"meta"
#define REPO_NODE_TYPE_REFERENCE		"ref"
#define REPO_NODE_TYPE_REVISION			"revision"
#define REPO_NODE_TYPE_SHADER			"shader"
#define REPO_NODE_TYPE_TEXTURE			"texture"
#define REPO_NODE_TYPE_TRANSFORMATION   "transformation"
#define REPO_NODE_TYPE_UNKNOWN			"unknown" // reserved UUID ext 00
//-----------------------------------------------------------------------------
#define REPO_REF_TYPE_S3	"s3"
//-----------------------------------------------------------------------------
