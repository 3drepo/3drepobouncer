/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo_log.h"

#if BOOST_VERSION > 105700
#include <boost/core/null_deleter.hpp>
#else
#include <boost/utility/empty_deleter.hpp>
#endif

#include <boost/log/trivial.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

using namespace repo::lib;

using text_sink = boost::log::sinks::synchronous_sink < boost::log::sinks::text_ostream_backend >;

extern RepoLog* singleton = nullptr;

RepoLog& RepoLog::getInstance()
{
	if (!singleton) {
		singleton = new RepoLog();
	}
	return *singleton;
}

// Cannot use repo_utils.h -> for some reason it switches the headers for :toupper.
std::string getEnvString(std::string const & envVarName)
{
	char* value = getenv(envVarName.c_str());
	return (value && strlen(value) > 0) ? value : "";
}

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(threadid, "ThreadID", boost::log::attributes::current_thread_id::value_type)

RepoLog::RepoLog()
{
	std::string logDir = getEnvString("REPO_LOG_DIR");
	logDir = logDir.empty() ? "./log/" : logDir;
	std::cout << "Logging directory is set to " << logDir << std::endl;
	this->logToFile(logDir);

	boost::log::add_console_log(std::cout);

	std::string debug = getEnvString("REPO_DEBUG");
	std::string verbose = getEnvString("REPO_VERBOSE");
	if (!verbose.empty())
	{
		this->setLoggingLevel(repo::lib::RepoLog::RepoLogLevel::TRACE);
	}
	else if (!debug.empty())
	{
		this->setLoggingLevel(repo::lib::RepoLog::RepoLogLevel::DEBUG);
	}
	else
	{
		this->setLoggingLevel(repo::lib::RepoLog::RepoLogLevel::INFO);
	}
}

RepoLog::~RepoLog()
{
}

std::string severityAsString(const RepoLog::RepoLogLevel& severity)
{
	switch (severity)
	{
	case RepoLog::RepoLogLevel::TRACE:
		return "trace";
	case RepoLog::RepoLogLevel::DEBUG:
		return "debug";
	case RepoLog::RepoLogLevel::INFO:
		return "info";
	case RepoLog::RepoLogLevel::WARNING:
		return "warning";
	case RepoLog::RepoLogLevel::ERR:
		return "error";
	case RepoLog::RepoLogLevel::FATAL:
		return "fatal";
	}
}

std::string getTimeAsString(bool nospaces = false)
{
	time_t rawtime;
	struct tm* timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	if (nospaces)
	{
		strftime(buffer, 80, "%Y-%m-%d-%Hh%Mm%Ss", timeinfo);
	}
	else 
	{
		strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
	}
	return std::string(buffer);
}

void RepoLog::log(
	const RepoLogLevel &severity,
	const std::string  &msg)
{
	auto full = "[" + getTimeAsString() + "]" + " <" + severityAsString(severity) + ">: " + msg;

	switch (severity)
	{
	case RepoLogLevel::TRACE:
		BOOST_LOG_TRIVIAL(trace) << full;
		break;
	case RepoLogLevel::DEBUG:
		BOOST_LOG_TRIVIAL(debug) << full;
		break;
	case RepoLogLevel::INFO:
		BOOST_LOG_TRIVIAL(info) << full;
		break;
	case RepoLogLevel::WARNING:
		BOOST_LOG_TRIVIAL(warning) << full;
		break;
	case RepoLogLevel::ERR:
		BOOST_LOG_TRIVIAL(error) << full;
		break;
	case RepoLogLevel::FATAL:
		BOOST_LOG_TRIVIAL(fatal) << full;
	}
}

RepoLog::record::~record()
{
	log.log(severity, stream.str());
}

RepoLog::record::record(RepoLog& log, const RepoLogLevel& severity):
	log(log),
	severity(severity)
{
}

void RepoLog::logToFile(const std::string &filePath)
{
	boost::filesystem::path logPath(filePath);
	std::string fileName;
	// a directory is given
	std::string name = getTimeAsString(true) + "_%N.log";
	fileName = (logPath / name).string();

	boost::log::add_file_log(
		boost::log::keywords::file_name = fileName,
		boost::log::keywords::rotation_size = 10 * 1024 * 1024,
		boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
		boost::log::keywords::auto_flush = true
	);
	boost::log::add_common_attributes();

	BOOST_LOG_TRIVIAL(info) << "Log file registered: " << filePath;
}

void RepoLog::setLoggingLevel(const RepoLogLevel &level)
{
	boost::log::trivial::severity_level loggingLevel;
	switch (level)
	{
	case repo::lib::RepoLog::RepoLogLevel::TRACE:
		loggingLevel = boost::log::trivial::trace;
		break;
	case repo::lib::RepoLog::RepoLogLevel::DEBUG:
		loggingLevel = boost::log::trivial::debug;
		break;
	case repo::lib::RepoLog::RepoLogLevel::INFO:
		loggingLevel = boost::log::trivial::info;
		break;
	case repo::lib::RepoLog::RepoLogLevel::WARNING:
		loggingLevel = boost::log::trivial::warning;
		break;
	case repo::lib::RepoLog::RepoLogLevel::ERR:
		loggingLevel = boost::log::trivial::error;
		break;
	case repo::lib::RepoLog::RepoLogLevel::FATAL:
		loggingLevel = boost::log::trivial::fatal;
		break;
	default:
		repoError << "Unknown log level: " << (int)level;
		return;
	}

	boost::log::core::get()->set_filter(
		boost::log::trivial::severity >= loggingLevel);
}

void RepoLog::subscribeBroadcaster(RepoBroadcaster *broadcaster) {
	boost::iostreams::stream<RepoBroadcaster> *streamptr =
		new boost::iostreams::stream<RepoBroadcaster>(*broadcaster);

#if BOOST_VERSION > 105700
	boost::shared_ptr< std::ostream > stream(
		streamptr, boost::null_deleter());
#else
	boost::shared_ptr< std::ostream > stream(
		streamptr, boost::empty_deleter());
#endif

	boost::shared_ptr< text_sink > sink = boost::make_shared< text_sink >();

	sink->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
	sink->locked_backend()->add_stream(stream);
	sink->locked_backend()->auto_flush(true);

	// Register the sink in the logging core
	boost::log::core::get()->add_sink(sink);
}

void RepoLog::subscribeListeners(
	const std::vector<RepoAbstractListener*> &listeners)
{
	/*
		FIXME: ideally, we should have a single broadcaster for the application
		and you should only need to add the listeners when you subscribe
		But the whole hackery of making a broadcaster to be a ostream
		to tap into the boost logger made this really difficult
		so for now, instantiate a new broadcaster everytime.
		And I will review this on a braver day...
		*/
	RepoBroadcaster *broadcaster = new RepoBroadcaster();

	for (auto listener : listeners)
		broadcaster->subscribe(listener);

	subscribeBroadcaster(broadcaster);
}
