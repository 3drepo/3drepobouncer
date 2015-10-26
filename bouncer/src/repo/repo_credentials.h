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

#pragma once

#include <string>
#include "repo_bouncer_global.h"

namespace repo {

/**
* Database credentials to be passed to controller in order to obtain DB token.
*/
class REPO_API_EXPORT RepoCredentials
{
    friend class RepoController;

public:

    RepoCredentials(std::string username,
                    std::string password,
                    std::string host = "localhost",
                    int port = 27017);

private :

    std::string username; //!< Username

    std::string password; //!< Plain-text password

    std::string host; //!< Server host

    int port; //!< Port number

};
} // end repo
