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

//Success
#define REPOERR_OK 0
//Bouncer failed to start - this never gets returned by bouncer client, but bouncer worker will return this
#define REPOERR_LAUNCHING_COMPUTE_CLIENT 1
//authentication to database failed
#define REPOERR_AUTH_FAILED 2
//unrecognised command
#define REPOERR_UNKNOWN_CMD 3
//unknown error (caught exception)
#define REPOERR_UNKNOWN_ERR 4
//failed to import file to scene
#define REPOERR_LOAD_SCENE_FAIL 5
//failed to generate stash graph
#define REPOERR_STASH_GEN_FAIL 6
//Scene uploaded, but missing texture
#define REPOERR_LOAD_SCENE_MISSING_TEXTURE 7
//invalid arguments to function
#define REPOERR_INVALID_ARG 8
//failed to generate federation
#define REPOERR_FED_GEN_FAIL 9
//Scene uploaded but missing some nodes
#define REPOERR_LOAD_SCENE_MISSING_NODES 10
//Failed to get file from project
#define REPOERR_GET_FILE_FAILED 11
//Failed to finish (i.e. crashed)
#define REPOERR_CRASHED 12
//Failed to read import parameters from file (Unity)
#define REPOERR_PARAM_FILE_READ_FAILED 13
//Failed to generate asset bundles (Unity)
#define REPOERR_BUNDLE_GEN_FAILED 14
//Scene loaded, has untriangulated meshes
#define REPOERR_LOAD_SCENE_INVALID_MESHES 15
//Failed to read the fail containing model information
#define REPOERR_ARG_FILE_FAIL 16
//The file loaded has no meshes
#define REPOERR_NO_MESHES 17
//Unsupported file extension
#define REPOERR_FILE_TYPE_NOT_SUPPORTED 18
//Failed to read model file
#define REPOERR_MODEL_FILE_READ 19
//Failed during assimp generation
#define REPOERR_FILE_ASSIMP_GEN 20
//Failed during IFC geometry generation
#define REPOERR_FILE_IFC_GEO_GEN 21
//Bim file version unsupported
#define REPOERR_UNSUPPORTED_BIM_VERSION 22
//FBX file version unsupported
#define REPOERR_UNSUPPORTED_FBX_VERSION 23
//Unsupported file version (generic)
#define REPOERR_UNSUPPORTED_VERSION 24
//Exceed the maximum amount fo nodes
#define REPOERR_MAX_NODES_EXCEEDED 25
//When ODA not compiled in but dgn import requested
#define REPOERR_ODA_UNAVAILABLE 26
//The specified view (if any) was not found (for Revit format)
#define REPOERR_VIEW_NOT_FOUND 27
//Failed reading configuration file
#define REPOERR_INVALID_CONFIG_FILE 28
//Process timed out (only used in bouncer_worker)
#define REPOERR_TIMEOUT 29
//When Synchro is not compiled within the library
#define REPOERR_SYNCHRO_UNAVAILABLE 30
//Synchro sequence exceed size of bson
#define REPOERR_SYNCHRO_SEQUENCE_TOO_BIG 31
//Imported successfully, but failed to upload it to the database/fileshares
#define REPOERR_UPLOAD_FAILED 32
//Trying to import toy project but failed (only used by bouncer_worker)
#define REPOERR_TOY_IMPORT_FAILED 33
//The scene was read successfully but failed during geometry processing due to content problem
#define REPOERR_GEOMETRY_ERROR 34
//Used in bouncer worker only
#define REPOERR_UNITY_LICENSE_INVALID 35
//When a component fails a 3drepo license check
#define ERRCODE_REPO_LICENCE_INVALID 36
//When a password protected or encrypted file is provided
#define REPOERR_FILE_IS_ENCRYPTED 37
//When image processing failed (only used by bouncer_worker)
#define REPOERR_IMAGE_PROCESSING_FAILED 38
//When the Ifc file uses a schema that is not supported by the importer
#define REPOERR_FILE_IFC_UNSUPPORTED_SCHEMA 39
//The specified view was found, but it is not a 3D view
#define REPOERR_VIEW_NOT_3D 40
