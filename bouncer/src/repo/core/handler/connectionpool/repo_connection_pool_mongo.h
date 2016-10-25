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

#pragma once

#include "../../../lib/repo_stack.h"
#include "../../../lib/repo_log.h"

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
				class MongoConnectionPool : repo::lib::RepoStack < mongo::DBClientBase * >
				{
				public:
					/**
					* Instantiate the pool of workers with a limited number of connections
					* @param numConnections number of connections
					*/
					MongoConnectionPool(
						const int &numConnections,
						mongo::ConnectionString dbAddress,
						mongo::BSONObj* auth) ;

					MongoConnectionPool() :maxSize(0){}

					~MongoConnectionPool();


					mongo::DBClientBase* getWorker()
					{
						return pop();
					}

					void returnWorker(mongo::DBClientBase *worker)
					{
						push(worker);
					}

					mongo::DBClientBase* pop();


					void push(mongo::DBClientBase *worker)
					{
						if (worker)
							RepoStack::push(worker);
					}
				private:
					mongo::DBClientBase* connectWorker(std::string &errMsg);

					const uint32_t maxSize;
					const mongo::ConnectionString dbAddress;
					const mongo::BSONObj *auth;
				};
			}
		} /* namespace handler */
	}
}
