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
*  Project setting BSON
*/

#include "repo_bson_builder.h"
#include "repo_bson_project_settings.h"
#include "../../../lib/repo_log.h"

using namespace repo::core::model;

double RepoProjectSettings::getEmbeddedDouble(
        const std::string &embeddedObjName,
        const std::string &fieldName,
        const double &defaultValue) const
{
    double value = defaultValue;
    if (hasEmbeddedField(embeddedObjName, fieldName))
    {
         value = (getObjectField(embeddedObjName)).getField(fieldName).numberDouble();
    }
    return value;
}

bool RepoProjectSettings::hasEmbeddedField(
            const std::string &embeddedObjName,
            const std::string &fieldName) const
{
    bool found = false;
    if (hasField(embeddedObjName))
    {
        found = (getObjectField(embeddedObjName)).hasField(fieldName);
    }
    return found;
}

// Deprecated
std::vector<bool> RepoProjectSettings::stringToPermissionsBool(std::string octal)
{
    std::vector<bool> perm;
    perm.resize(12);
    uint8_t mask[3] = { READVAL, WRITEVAL, EXECUTEVAL };

    if (octal.size() >= 4)
    {
        for (int i = 0; i < 4; i++)
        {
            uint32_t p = std::stoi(octal.substr(i, 1));
            for (int j = 0; j < 3; j++)
            {
                uint32_t ind = i * 3 + j;
                perm[ind] = p & mask[j];
            }

        }
    }
    else
    {
        repoError << "ProjectSettings - stringToPermissionsBool : size of parameter is smaller than 4.";
    }

    return perm;
}

