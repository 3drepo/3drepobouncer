/**
*  Copyright (C) 2019 3D Repo Ltd
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
#include "date/tz.h"

namespace repo {
	namespace lib {

        class TimeZoneConverter
        {
        public:
            /*
            * The first call to date::locate_zone in this constructor will take
            * additional time as database is being loaded by the date lib. 
            */
            TimeZoneConverter(std::string const timeZoneName):
                timeZone(date::locate_zone(timeZoneName)),
                utcTimeZone(date::locate_zone("Etc/UTC"))
            {};

            std::chrono::seconds timeZoneEpochToUtcEpoch(std::chrono::seconds tzEpoch)
		    {
                // convert the duration since unix epoch to local time zone
                auto localTime = date::zoned_seconds{
                    timeZone,
                    date::local_seconds(tzEpoch) };
                // return the converted duration since unix epoch in utc time zone
                return date::zoned_seconds{ 
                    utcTimeZone, 
                    localTime }.get_sys_time().time_since_epoch();
		    }

        private:

            date::time_zone const* const timeZone;
            date::time_zone const* const utcTimeZone;

        };

	}
}