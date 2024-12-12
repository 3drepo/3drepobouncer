/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include "repo_global_manager.h"
#include "repo/lib/repo_exception.h"
#include <stack>

static bool initialised = false;
static std::stack<std::unique_ptr<repo::RepoGlobalManager::Destructor>> destructors;

REPO_API_EXPORT repo::RepoGlobalManager::RepoGlobalManager()
{
	if (initialised) {
		throw new repo::lib::RepoException("Global manager already constructed.");
	}
	initialised = true;
}

REPO_API_EXPORT repo::RepoGlobalManager::~RepoGlobalManager()
{
	while (!destructors.empty()) {
		destructors.top()->operator void();
		destructors.pop();
	}
}

void repo::RepoGlobalManager::addDestructor(std::unique_ptr<Destructor> d)
{
	destructors.push(std::move(d));
}