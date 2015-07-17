#pragma once





#include "../../../lib/repo_stack.h"



#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Windows.h>

#define strcasecmp _stricmp
#endif

#include <mongo/client/dbclient.h>

namespace repo{
	namespace core{
		namespace handler {
			namespace connectionPool{
				class MongoConnectionPool : repo::lib::RepoStack <mongo::DBClientBase * >
				{
				public:
					/**
					* Instantiate the pool of workers with a limited number of connections
					* @param numConnections number of connections
					*/
					MongoConnectionPool(
						const int &numConnections,
						mongo::ConnectionString dbAddress,
						mongo::BSONObj* auth) :
						maxSize(numConnections),
						dbAddress(dbAddress),
						auth(auth)
					{
						//push one connected worker to ensure valid connection
						//so the caller can handle the exceptions appropriately
						mongo::DBClientBase *worker = dbAddress.connect(std::string());
						worker->auth(*auth);

						push(worker);
						for (int i = 1; i < numConnections; i++)
						{
							//push a bunch of null pointers into the stack
							//We will connect these as required
							push(0);
						}
					}

					MongoConnectionPool():maxSize(0){}


					~MongoConnectionPool()
					{
						delete auth;
					}

					mongo::DBClientBase* getWorker()
					{
						mongo::DBClientBase* worker = pop();

						if (!worker)
						{
							//worker was never used, instantiate it with a connection
							worker = connectWorker(std::string());
						}
						else
						{
							//check worker is still connected
							if (!worker->isStillConnected())
							{
								worker = connectWorker(std::string());
							}
						}

						return worker;
					}

					void returnWorker(mongo::DBClientBase *worker)
					{
						if (worker)
							push(worker);
					}

				private:
					mongo::DBClientBase* connectWorker(std::string &errMsg)
					{
						mongo::DBClientBase *worker = dbAddress.connect(errMsg);
						worker->auth(*auth);
						return worker;
					}


					const uint32_t maxSize;
					const mongo::ConnectionString dbAddress;
					const mongo::BSONObj *auth;

				};
			
			}
		} /* namespace handler */
	}
}

