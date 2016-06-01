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
* Essentially a wrapper around boost logger in case we want to use something else in the future
*/

#pragma once

#include <string>
#include <boost/log/trivial.hpp>

#include "repo_broadcaster.h"
#include "repo_listener_abstract.h"
#include "../repo_bouncer_global.h"

//------------------------------------------------------------------------------
// Logging macros - to avoid having to write all that everytime to log something

//The external libraries need to use these 2 because they don't seem to share the same instance of log
#define repoLog(MSG) repo::lib::RepoLog::getInstance().log(repo::lib::RepoLog::RepoLogLevel::INFO, MSG)
#define repoLogDebug(MSG) repo::lib::RepoLog::getInstance().log(repo::lib::RepoLog::RepoLogLevel::DEBUG, MSG)
#define repoLogError(MSG) repo::lib::RepoLog::getInstance().log(repo::lib::RepoLog::RepoLogLevel::ERR, MSG)

//internal classes should all be using this.
#define repoTrace BOOST_LOG_TRIVIAL(trace)
#define repoDebug BOOST_LOG_TRIVIAL(debug)
#define repoInfo BOOST_LOG_TRIVIAL(info)
#define repoWarning BOOST_LOG_TRIVIAL(warning)
#define repoError BOOST_LOG_TRIVIAL(error)
#define repoFatal BOOST_LOG_TRIVIAL(fatal)

namespace repo{
	namespace lib{
		class REPO_API_EXPORT RepoLog
		{
		public:

			/**
			* LOGGING LEVEL
			*/
			enum class RepoLogLevel { TRACE, DEBUG, INFO, WARNING, ERR, FATAL };

			~RepoLog();

			static RepoLog &getInstance()
			{
				static RepoLog log = RepoLog();
				return log;
			}

			/**
			* Log a message(carriage return is not needed!)
			* @param severity severity of the message
			* @msg message itself
			*/
			void log(
				const RepoLogLevel &severity,
				const std::string  &msg);

			/**
			* Log to a specific file
			* @param filePath path to file
			*/
			void logToFile(const std::string &filePath);

			/**
			* Configure how verbose the log should be
			* The levels of verbosity are:
			* TRACE - log all messages
			* DEBUG - log messages of level debug or above (use for debugging)
			* INFO - log messages of level info or above (use to filter debugging messages but want informative logging)
			* WARNING - log messages of level warning or above
			* ERROR - log messages of level error or above
			* FATAL - log messages of level fatal or above
			* @param level specify logging level
			*
			*/
			void setLoggingLevel(const RepoLogLevel &level);

			/**
			* Subscribe the given broadcaster to the logging system
			* @param broadcaster the broadcaster to subscribe
			*/
			void subscribeBroadcaster(RepoBroadcaster *broadcaster);

			/**
			* Subscribe a RepoAbstractLister to logging messages
			* @param listener object to subscribe
			*/
			void subscribeListeners(
				const std::vector<RepoAbstractListener*> &listeners);

		private:

			RepoLog();
		};
	}
}
