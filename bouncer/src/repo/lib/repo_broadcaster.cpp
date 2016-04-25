#include "repo_broadcaster.h"
#include <boost/algorithm/string.hpp>
using namespace repo::lib;

RepoBroadcaster::RepoBroadcaster()
{
}

RepoBroadcaster::~RepoBroadcaster()
{
}

//RepoBroadcaster* RepoBroadcaster::getInstance()
//{
//
//	static RepoBroadcaster *broadCaster = new RepoBroadcaster();
//	return broadCaster;
//}

std::streamsize RepoBroadcaster::write(const char* s, std::streamsize n)
{
	std::string msg(s, n);

	size_t firstPos = msg.find_first_of('%') + 1;
	size_t secPos = msg.find_first_of('%', firstPos + 1);

	std::string actualMessage = msg;
	if (firstPos != std::string::npos && secPos != std::string::npos)
	{
		actualMessage = msg.substr(secPos + 1);
	}

	boost::trim(actualMessage); //remove trailing spaces/carriage returns

	if (actualMessage.empty()) return n; //don't bother broadcasting if the actual message is empty

	for (const auto &listener : subscribers)
	{
		listener->messageGenerated(msg);
	}
	return n;
}