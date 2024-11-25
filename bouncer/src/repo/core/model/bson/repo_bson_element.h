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
*  Wrapper around mongo BSON element. This is needed to abstract away the mongo dependency,
* which we should eventually get rid of and use a standalone library
*/

#pragma once
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Windows.h>
#endif

#include "repo/repo_bouncer_global.h"
#include "repo/lib/repo_log.h"
#include "repo/lib/datastructure/repo_variant.h"

#include <bsoncxx/document/element.hpp>

namespace repo {
	namespace core {
		namespace model {
			//type of element
			enum class REPO_API_EXPORT ElementType{
				ARRAY, UUID, BINARY, BOOL, DATE,
				OBJECTID, DOUBLE, INT, LONG, OBJECT, STRING, UNKNOWN
			};
			class RepoBSON;
			class REPO_API_EXPORT RepoBSONElement : private bsoncxx::document::element
			{
				friend class RepoBSONBuilder;
				friend class RepoBSON;

			public:
				/**
				* Construct a RepoBSONElement base on a mongo element
				* @param mongo BSON element
				*/
				RepoBSONElement(bsoncxx::document::element ele) : bsoncxx::document::element(ele) {}

				/**
				* Destructor
				*/
				~RepoBSONElement();

				/**
				* get the type of the element
				* @return returns the type of the element using enum Type specified above
				*/
				ElementType type() const;

				std::string String() const;

				RepoBSON Object() const;

				time_t TimeT() const;

				tm Tm() const;

				bool Bool() const;

				int Int() const;

				long long Long() const;

				double Double() const;

				size_t size() const;

				repo::lib::RepoUUID UUID() const;

				repo::lib::RepoVariant repoVariant() const;

				bool operator==(const RepoBSONElement& other) const;

				bool operator!=(const RepoBSONElement& other) const;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
