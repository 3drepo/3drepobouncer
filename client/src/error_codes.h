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
#define REPOERR_FILE_READ_FAILED 13
//Failed to generate asset bundles (Unity)
#define REPOERR_BUNDLE_GEN_FAILED 14
//Scene loaded, has untriangulated meshes
#define REPOERR_LOAD_SCENE_INVALID_MESHES 15

