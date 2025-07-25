/*
*  Copyright(C) 2015 3D Repo Ltd
*
*  This program is free software : you can redistribute it and / or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or(at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.If not, see <http://www.gnu.org/licenses/>.
*/

/**
* BSON object builder. Currently it does nothing more than inherit mongo.
* It is a layer of abstraction between 3D Repo and mongo.
* In the future this may be replaced by utilising a BSON library
*/

#pragma once

#include "repo_bson.h"
#include "repo/lib/datastructure/repo_matrix.h"
#include "repo/lib/datastructure/repo_uuid.h"
#include "repo/lib/datastructure/repo_bounds.h"
#include "repo/lib/datastructure/repo_variant.h"
#include "repo/lib/datastructure/repo_structs.h"
#include <boost/variant/static_visitor.hpp>
#include <string>
#include <ctime>
#include <bsoncxx/builder/core.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT RepoBSONBuilder : private bsoncxx::builder::core
			{
			public:
				RepoBSONBuilder();
				~RepoBSONBuilder();

				using BinMapping = RepoBSON::BinMapping;

				BinMapping& mapping()
				{
					return binMapping;
				}

			private:
				BinMapping binMapping;

			public:

				// (Keep the templated definition in the header so the compiler
				// can create concrete classes with the templated types when they
				// are used)

				template<typename T>
				void appendLargeArray(std::string name, const std::vector<T>& data)
				{
					appendLargeArray(name, &data[0], data.size() * sizeof(data[0]));
				}

				void appendLargeArray(std::string name, const void* data, size_t size);

				/**
				* Append a vector as a bson array.
				* @param label label of the array
				* @param vec vector to append
				*/
				template <class T>
				void appendArray(
					const std::string &label,
					const std::vector<T> &vec)
				{
					core::key_owned(label);
					append(vec);
				}

				template <class T>
				void appendIteratable(
					const std::string& label,
					T begin,
					T end
				)
				{
					core::key_owned(label);
					core::open_array();
					for (auto it = begin; it != end; it++) {
						append(*it);
					}
					core::close_array();
				}

				template<class T>
				void append(
					const std::string& label,
					const T& item)
				{
					core::key_owned(label);
					append(item);
				}

				template<class V>
				void append(
					const std::string& label,
					const std::vector<V> items)
				{
					appendArray(label, items);
				}

				void appendRepoVariant(
					const std::string& label,
					const repo::lib::RepoVariant& item);

				void appendElements(RepoBSON bson);

				void appendElementsUnique(RepoBSON bson);

				void appendTimeStamp(std::string label);

				void appendTime(std::string label, const tm& t);

				// The following two methods consider a int64_t as a posix timestamp (the
				// number of seconds) since the unix epoch. The field type will be a mongo
				// date object.

				void appendTime(std::string label, const int64_t& ts);

				void appendTime(const int64_t& ts);

				/**
				* Appends a Vector but as an object, instead of an array.
				*/
				void appendVector3DObject(
					const std::string& label,
					const repo::lib::RepoVector3D& vec
				);

				/**
				* builds the BSON object as RepoBSON given the information within the builder
				* @return returns a RepoBSON object with the fields given.
				*/
				RepoBSON obj();

			private:
				// This exists so the base method (which has a return type) counts as
				// an overload alongside those for the special types below.
				template<typename T>
				void append(const T& item)
				{
					core::append(item);
				}

				template<typename T>
				void append(const std::vector<T> items)
				{
					core::open_array();
					for (const auto& v : items)
					{
						append(v);
					}
					core::close_array();
				}

				void append(const tm& tm);

				void append(const repo::lib::RepoUUID& uuid);

				void append(const repo::lib::RepoBounds& bounds);

				void append(const repo::lib::RepoVector3D& vec);

				void append(const repo::lib::RepoMatrix& mat);

				void append(const repo::lib::repo_color3d_t& col);

				void append(const repo::lib::repo_color4d_t& col);

				void append(const repo::lib::RepoVariant& variant);

				void append(const RepoBSON& obj);

				/**
				* @brief Append a UUID into the builder
				* This is a is a wrapper around appendBinData from mongo as
				* there is no support to append boost::uuid directly
				* @param label label of the field
				* @param uuid UUID
				*/
				void appendUUID(
					const std::string &label,
					const repo::lib::RepoUUID &uuid);

				// Visitor class to process the metadata variant correctly
				class AppendVisitor : public boost::static_visitor<> {
				public:

					AppendVisitor(RepoBSONBuilder& aBuilder) : builder(aBuilder) {}

					void operator()(const bool& b) const {
						builder.append(b);
					}

					void operator()(const int& i) const {
						builder.append(i);
					}

					void operator()(const int64_t& ll) const {
						builder.append(ll);
					}

					void operator()(const double& d) const {
						builder.append(d);
					}

					void operator()(const std::string& s) const {
						builder.append(s);
					}

					void operator()(const tm& t) const {
						builder.append(t);
					}

					void operator()(const repo::lib::RepoUUID& u) const {
						builder.append(u);
					}

				private:
					RepoBSONBuilder& builder;
				};
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
