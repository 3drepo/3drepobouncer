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
#include <mongo/bson/bson.h>

#include "../../../repo_bouncer_global.h"
#include "../../../lib/repo_log.h"
#include "../../../lib/datastructure/repo_variant.h"

namespace repo {
	namespace core {
		namespace model {
			//type of element
			enum class REPO_API_EXPORT ElementType{
				ARRAY, UUID, BINARY, BOOL, DATE,
				OBJECTID, DOUBLE, INT, LONG, OBJECT, STRING, UNKNOWN
			};
			class REPO_API_EXPORT RepoBSONElement :
				private mongo::BSONElement
			{
				friend class RepoBSONBuilder;
				friend class RepoBSON;

			public:

				/**
				* Default constructor
				*/
				RepoBSONElement() : mongo::BSONElement() {}

				/**
				* Construct a RepoBSONElement base on a mongo element
				* @param mongo BSON element
				*/
				RepoBSONElement(mongo::BSONElement ele) : mongo::BSONElement(ele) {}

				/**
				* Destructor
				*/
				~RepoBSONElement();

				/**
				* get the type of the element
				* @return returns the type of the element using enum Type specified above
				*/
				ElementType type() const;

				std::vector<RepoBSONElement> Array()
				{
					//FIXME: potentially slow.
					//This is done so we can hide mongo representation from the bouncer world.
					std::vector<RepoBSONElement> arr;

					if (!eoo())
					{
						std::vector<mongo::BSONElement> mongoArr = mongo::BSONElement::Array();
						arr.reserve(mongoArr.size());

						for (auto const &ele : mongoArr)
						{
							arr.push_back(RepoBSONElement(ele));
						}
					}

					return arr;
				}

				std::string String() const{
					return mongo::BSONElement::String();
				}

				mongo::BSONObj embeddedObject() const {
					return mongo::BSONElement::embeddedObject();
				}

				mongo::Date_t date() const {
					return mongo::BSONElement::date();
				}

				bool Bool() const {
					return mongo::BSONElement::Bool();
				}

				int Int() const{
					return mongo::BSONElement::Int();
				}

				long long Long() const {
					return mongo::BSONElement::Long();
				}

				double Double() const {
					return mongo::BSONElement::Double();
				}

				size_t size() const {
					return mongo::BSONElement::size();
				}

				const char* binData(int &length) const {
					return mongo::BSONElement::binData(length);
				}

				repo::lib::RepoVariant repoVariant() const;

				mongo::BSONElement toMongoElement() const {
					return *this;
				}

				inline bool operator==(const RepoBSONElement & other) const {
					return mongo::BSONElement::operator==(other);
				}

				inline bool operator!=(const RepoBSONElement other) const {
					return mongo::BSONElement::operator!=(other);
				}

				std::string toString() const {
					return mongo::BSONElement::toString();
				}

				bool eoo() const {
					return mongo::BSONElement::eoo();
				}

				bool isNull() const {
					return mongo::BSONElement::isNull();
				}
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
