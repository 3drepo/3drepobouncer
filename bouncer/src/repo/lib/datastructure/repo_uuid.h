/**
*  Copyright (C) 2016 3D Repo Ltd
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

#include "repo/repo_bouncer_global.h"
#include <boost/uuid/uuid.hpp>
#include <iterator>
#include <vector>
#include <ostream>

namespace repo {
	namespace lib {
		class REPO_API_EXPORT RepoUUID
		{
		public:

			RepoUUID(const boost::uuids::uuid &id) : id(id) {}

			RepoUUID(const RepoUUID &other) : id(other.id) {}

			RepoUUID(const std::string &stringRep = defaultValue);

			static RepoUUID createUUID();

			/**
			* Get the underlying binary data from the UUID
			* @returns the UUID in binary format
			*/
			std::vector<uint8_t> data() const { return std::vector<uint8_t>(id.begin(), id.end()); }

			bool isDefaultValue() const {
				return toString() == defaultValue;
			}

			size_t getHash() const;

			/**
			* Converts a RepoUUID to string
			* @return a string representation of repoUUID
			*/
			std::string toString() const;

			static const std::string defaultValue;

			const boost::uuids::uuid& getInternalID() const { return id; }

		private:
			boost::uuids::uuid id;
		};

		inline std::ostream& operator<<(std::ostream& stream, const RepoUUID& uuid)
		{
			stream << uuid.toString();
			return stream;
		}

		inline bool operator==(const RepoUUID& id1, const RepoUUID& id2)
		{
			return id1.getInternalID() == id2.getInternalID();
		}

		inline bool operator<(const RepoUUID& id1, const RepoUUID& id2)
		{
			return id1.getInternalID() < id2.getInternalID();
		}

		inline bool operator!=(const RepoUUID& id1, const RepoUUID& id2)
		{
			return !(id1 == id2);
		}

		inline bool operator>(const RepoUUID& id1, const RepoUUID& id2)
		{
			return id2 < id1;
		}

		inline bool operator<=(const RepoUUID& id1, const RepoUUID& id2)
		{
			return !(id2 < id1);
		}

		inline bool operator>=(const RepoUUID& id1, const RepoUUID& id2)
		{
			return !(id1 < id2);
		}

		struct RepoUUIDHasher
		{
			std::size_t operator()(const RepoUUID& uid) const;
		};
	}
}
