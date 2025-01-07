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

#include "repo_system_services.h"

#include <iostream>

// Usage of ODA should be bounded by calls to odActivate and
// odCleanUpStaticData. odCleanUpStaticData however can only be called once in
// the lifetime of a process.
// In practice, because we don't make assumptions about how many times ODA may
// be used, odCleanUpStaticData is never called, and we rely on the operating
// system to handle any resources the ODA activation system may leak.
// 
// The ODA system services themselves, expressed as RxSystemServicesImpl and
// other classes, should be cleaned up as normal.

static bool initialised = false;

RepoSystemServices::RepoSystemServices()
{
	if (!initialised) {
		odActivate(
			#include "OdActivationInfo"
		);
		initialised = true;
	}
}