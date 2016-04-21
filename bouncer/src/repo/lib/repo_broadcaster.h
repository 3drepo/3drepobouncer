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
* A Broadcasting class that subscribes to the Repo Bouncer logging system (boost::log)
* and broadcast to all its subscriber the message. This pretends to be a sink
* for a iostream, so when the write function is called, it's actually calling all of it's
* subscriber's messageGenerated() function;
* Inspiration from http://stackoverflow.com/questions/533038/redirect-stdcout-to-a-custom-writer
*/

#pragma once
#include <vector>
#include <iostream>

#include <algorithm>                       // copy, min
#include <iosfwd>                          // streamsize
#include <boost/iostreams/categories.hpp>  // sink_tag

#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include "repo_listener_abstract.h"

namespace repo{
	namespace  lib{
		class RepoBroadcaster : public boost::iostreams::sink
		{
		public:
			~RepoBroadcaster();

			//static RepoBroadcaster* getInstance();
			/**
			* FIXME: should be a private constructor - singleton class.
			*/
			RepoBroadcaster();

			/**
			* Subscribe to the broadcaster
			* @param subscriber An object to notify when there are new messages
			*/
			void subscribe(RepoAbstractListener *subscriber)
			{
				if (subscriber)
					subscribers.push_back(subscriber);
			}

			/**
			* When called, it will notify all subscribers of the message.
			*/
			std::streamsize write(const char* s, std::streamsize n);

		private:

			std::vector<RepoAbstractListener *> subscribers;
		};
	}
}
