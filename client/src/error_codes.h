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
//invalid arguments to function
#define REPOERR_INVALID_ARG 1
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