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
* Abstract listener class that any object wishing to subscript to the library logger
* will have to implement.
*/

#pragma once

#include "repo_listener_abstract.h"
#include <algorithm>
#include <iostream>
#include <ctime>

namespace repo{
	namespace lib{
		class LogToStdout : public RepoAbstractListener
		{
		public:
			LogToStdout() {}
			~LogToStdout(){};

			virtual void messageGenerated(const std::string &message)
			{
				size_t firstPos = message.find_first_of('%') + 1;
				size_t secPos = message.find_first_of('%', firstPos + 1);
				std::string severity = message.substr(firstPos, secPos - firstPos);
				std::transform(severity.begin(), severity.end(), severity.begin(), ::toupper);
				const std::string actualMessage = message.substr(secPos + 1);

				std::cout << "[" << getTimeAsString() << "][" << severity << "]: " << actualMessage;
			};

		private:

			static std::string getTimeAsString()
			{
				time_t rawtime;
				struct tm * timeinfo;
				char buffer[80];

				time(&rawtime);
				timeinfo = localtime(&rawtime);

				strftime(buffer, 80, "%d-%m-%Y %I:%M:%S", timeinfo);
				return std::string(buffer);
			}
		};
	}
}
