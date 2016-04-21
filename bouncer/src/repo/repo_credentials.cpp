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

#include "repo_credentials.h"

using namespace repo;

RepoCredentials::RepoCredentials(
	std::string alias,
	std::string host,
	int port,
	std::string authenticationDatabase,
	std::string username,
	std::string password)
	: alias(alias)
	, host(host)
	, port(port)
	, authenticationDatabase(authenticationDatabase)
	, username(username)
	, password(password)
{
}