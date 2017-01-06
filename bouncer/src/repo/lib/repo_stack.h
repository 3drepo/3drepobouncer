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
* A (attempt to be) thread safe stack essentially that would push/pop items as required
* Incredibly weird, I know.
*/

#pragma once

#include <stack>

#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include "repo_log.h"

namespace repo{
	namespace lib{
		template <class T>
		class RepoStack
		{
		public:
			RepoStack(
				const int32_t &maxRetry = -1,
				const uint32_t &msTimeOut = 50)
				: maxRetry(maxRetry)
				, msTimeOut(msTimeOut){}
			~RepoStack(){}

			void push(T*& item) {
				boost::mutex::scoped_lock lock(pushMutex);
				stack.push_back(item);
			}
			T* pop() {
				//order potentially matters. Think before shuffling!
				boost::mutex::scoped_lock popLock(popMutex); //stopping anyone from popping
				boost::mutex::scoped_lock pushLock(pushMutex); //stopping anyone from pushing

				uint8_t count = 0;
				while (stack.empty())
				{
					//stack is empty. Release the push lock and try again in 50ms
					pushLock.unlock();
					boost::this_thread::sleep(boost::posix_time::milliseconds(msTimeOut * (count + 1 * 10)));
					pushLock.lock();
					if (maxRetry >= 0 && count++ > maxRetry) break;
				}

				if (!stack.empty())
				{
					T* item = (T*)stack.back();
					stack.pop_back();
					return item;
				}

				repoTrace << "Given up. returning nullptr";
				return nullptr;
			}

			/**
			* empty the stack and return all its elements in a vector
			* @return vector of T
			*/
			std::vector<T*> empty()
			{
				boost::mutex::scoped_lock popLock(popMutex); //stopping anyone from popping
				boost::mutex::scoped_lock pushLock(pushMutex); //stopping anyone from pushing
				std::vector<T*> clone = stack;
				stack.clear();
				return clone;
			}

		private:
			std::vector<T*> stack;
			const int32_t maxRetry;
			const uint32_t msTimeOut;
			mutable boost::mutex pushMutex;
			mutable boost::mutex popMutex;
		};
	}
}
