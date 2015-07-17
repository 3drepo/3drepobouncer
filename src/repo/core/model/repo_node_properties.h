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
* Holds specific information about nodes
*/

#ifndef REPO_NODE_TYPES
#define REPO_NODE_TYPES

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
//-----------------------------------------------------------------------------
#define REPO_NODE_TYPE_ANIMATION		"animation"
#define REPO_NODE_TYPE_BONE				"bone"
#define REPO_NODE_TYPE_CAMERA			"camera"
#define REPO_NODE_TYPE_COMMENT			"comment"
#define REPO_NODE_TYPE_LIGHT			"light"
#define REPO_NODE_TYPE_LOCK				"lock"
#define REPO_NODE_TYPE_MAP          	"map"
#define REPO_NODE_TYPE_MATERIAL			"material"
#define REPO_NODE_TYPE_MESH          	"mesh"
#define REPO_NODE_TYPE_METADATA     	"meta"
#define REPO_NODE_TYPE_REFERENCE		"reference"
#define REPO_NODE_TYPE_REVISION			"revision"
#define REPO_NODE_TYPE_SHADER			"shader"
#define REPO_NODE_TYPE_TEXTURE			"texture"
#define REPO_NODE_TYPE_TRANSFORMATION   "transformation"
#define REPO_NODE_TYPE_UNKNOWN			"unknown" // reserved UUID ext 00
//-----------------------------------------------------------------------------

#endif
