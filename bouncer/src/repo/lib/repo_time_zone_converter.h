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
#ifdef SYNCHRO_SUPPORT
#include <date/tz.h>
#endif

namespace repo {
	namespace lib {
		class TimeZoneConverter
		{
		public:

			TimeZoneConverter(const std::string &timeZoneName)
#ifdef SYNCHRO_SUPPORT
				// Current this is only used by synchro imports. So #defs are introduced to reduce unnecessary external libraries.
				: timeZone(date::locate_zone(timeZoneName))
#endif
			{};

			uint64_t timeZoneEpochToUtcEpoch(const uint64_t &tzEpoch)
			{
#ifdef SYNCHRO_SUPPORT
				// convert the duration since unix epoch to local time zone
				auto localTime = date::zoned_seconds{
					timeZone, date::local_seconds(toChronoSec(tzEpoch)) };
				// return the converted duration since unix epoch in utc time zone
				return fromChronoSec(date::zoned_seconds{
					utcTimeZone,
					localTime }.get_sys_time().time_since_epoch());
#else
				return tzEpoch;
#endif
			}

		private:

			const date::time_zone *timeZone;
			const date::time_zone *utcTimeZone = date::locate_zone("Etc/UTC");

			std::chrono::seconds toChronoSec(const uint64_t &input) {
				return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::from_time_t(input).time_since_epoch());
			}

			uint64_t fromChronoSec(const std::chrono::seconds &input) {
				return input.count();
			}
		};
	}
}