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

#pragma once

#include "repo/repo_bouncer_global.h"
#include <memory>

namespace repo {
	/**
	* RepoGlobalManager is a helper class to control the static initialisation
	* order problem for depedencies that cannot be de-globalised. Such
	* dependencies cannot rely on the global object destructor pattern, and
	* the only option is to explicitly destruct them, which is what this class
	* does. An instance of this class should be created at a scope such that
	* its destructor runs right before program exits.
	*/
	class RepoGlobalManager
	{
	public:
		REPO_API_EXPORT RepoGlobalManager();
		REPO_API_EXPORT ~RepoGlobalManager();

		struct Destructor {
			virtual operator void() = 0;
		};

		// Will call the function when the global scope exits
		static void addDestructor(std::unique_ptr<Destructor>);
	};
}