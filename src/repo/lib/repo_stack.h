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

namespace repo{
	namespace lib{
		template <class T>
		class RepoStack
		{
		public:
			RepoStack(){}
			~RepoStack(){}
		protected:


				void push(const T& item) {
					boost::mutex::scoped_lock lock(pushMutex);
					stack.push(item);
				}
				T pop() {

					//order potentially matters. Think before shuffling!
					boost::mutex::scoped_lock popLock(popMutex); //stopping anyone from popping
					boost::mutex::scoped_lock pushLock(pushMutex); //stopping anyone from pushing

					while (stack.empty())
					{
						//stack is empty. Release the push lock and try again in 5s
						pushLock.release();							
						boost::this_thread::sleep(boost::posix_time::seconds(5));
						pushLock.lock();
					}

					T item = (T)stack.top();
					stack.pop();
					return item;
				}

		private:
			std::stack<T> stack;
			mutable boost::mutex pushMutex;
			mutable boost::mutex popMutex;


		};
	}
}


