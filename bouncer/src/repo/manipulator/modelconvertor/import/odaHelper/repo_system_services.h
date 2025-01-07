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

/**
* ODA library usage should bookmarked by the odActivate and odCleanUpStaticData
* functions.
* However, odCleanUpStaticData can only be called once in the lifetime of an
* application. ODA performs these operations in the ExSystemServices base class,
* meaning it is not possible for an application to let an ExSystemServices
* subclass be constructed, destructed, and then to re-initialise ODA.
* In theory, it is possible for an application to completely unload and reload
* ODA, but this has linking nuances on Linux.
* 
* See this post for additional details:
* https://forum.opendesign.com/showthread.php?24174-ODA-Drawings-SDK-v25-8-fails-to-odInitialize()-on-the-second-time
*
* To support the case where multiple systems services may be used in series, such
* as in the unit tests, the RepoSystemServices class initialises ODA on-demand,
* and then using a static object internally, cleans up once at the end of
* the application. If ODA is never initialised, the class does nothing.
*/

#include <OdaCommon.h>
#include <ExSystemServices.h>

// Use this class instead of ExSystemServices in order to allow multiple
// instances of it to be constructed and destructed within the lifetime
// 
class RepoSystemServices : public RxSystemServicesImpl
{
public:
	RepoSystemServices();
};
