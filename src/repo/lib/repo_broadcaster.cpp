#include "repo_broadcaster.h"

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

	for (const auto &listener : subscribers)
	{
		listener->messageGenerated(msg);

	}
	return n;
}
