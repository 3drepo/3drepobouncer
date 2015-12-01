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
*  Role setting BSON
*/

#pragma once
#include "repo_bson.h"
#include <vector>
#include <string>

namespace repo {
namespace core {
namespace model {

class REPO_API_EXPORT RepoRoleSettings : public RepoBSON
{

public:

    RepoRoleSettings() : RepoBSON() {}

    RepoRoleSettings(RepoBSON bson) : RepoBSON(bson){}

    ~RepoRoleSettings() {}

public :

    /**
    * Get the color of the role from this settings
    * @return returns role color as #hex string
    */
    std::string getColor() const
    {
        return getStringField(REPO_LABEL_COLOR);
    }

    /**
    * Get the description of the role from this settings
    * @return returns role description as string
    */
    std::string getDescription() const
    {
        return getStringField(REPO_LABEL_DESCRIPTION);
    }

    /**
    * Get the vector of available modules from this settings
    * @return returns modules as vector of strings
    */
    std::vector<std::string> getModules() const
    {
        return getStringArray(REPO_LABEL_MODULES);
    }

    /**
    * Get the name of the role for this settings
    * @return returns role name as string
    */
    std::string getName() const
    {
        return getStringField(REPO_LABEL_ID);
    }
};
}// end namespace model
} // end namespace core
} // end namespace repo

