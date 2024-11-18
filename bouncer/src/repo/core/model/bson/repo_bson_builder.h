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

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Windows.h>

#define strcasecmp _stricmp
#endif

#include "repo_bson.h"
#include "repo/lib/datastructure/repo_matrix.h"
#include "repo/lib/datastructure/repo_uuid.h"
#include "repo/lib/datastructure/repo_variant.h"
#include <boost/variant/static_visitor.hpp>
#include <string>
#include <ctime>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT RepoBSONBuilder : private bsoncxx::builder::basic::document
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
				* Append a vector as object into the bson. BSON arrays are
				* documents where each field is a monotonically increasing
				* integer. This method creates such a document and inserts
				* it into the builder.
				* @param label label of the array
				* @param vec vector to append
				*/
				template <class T>
				void appendArray(
					const std::string &label,
					const std::vector<T> &vec)
				{
					RepoBSONBuilder array;
					for (unsigned int i = 0; i < vec.size(); ++i)
						array.append(std::to_string(i), vec[i]);
					append(label, array.obj());
				}

				void appendArray(
					const std::string& label,
					const RepoBSON& bson);

				void appendRepoVariant(
					const std::string& label,
					const repo::lib::RepoVariant& item);

				template<class T>
				void append(
					const std::string& label,
					const T& item)
				{
					bsoncxx::builder::basic::document::append(bsoncxx::builder::basic::kvp(label, item));
				}

				template<class V>
				void append(
					const std::string& label,
					const std::vector<V> items)
				{
					appendArray(label, items);
				}

				void appendElements(RepoBSON bson);

				void appendElementsUnique(RepoBSON bson);

				void appendTimeStamp(std::string label);

				void appendTime(std::string label, const int64_t& ts);

				void appendTime(std::string label, const tm& t);

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

					AppendVisitor(RepoBSONBuilder& aBuilder, const std::string& aLabel) : builder(aBuilder), label(aLabel) {}

					void operator()(const bool& b) const {
						builder.append(label, b);
					}

					void operator()(const int& i) const {
						builder.append(label, i);
					}

					void operator()(const long long& ll) const {
						builder.append(label, ll);
					}

					void operator()(const double& d) const {
						builder.append(label, d);
					}

					void operator()(const std::string& s) const {
						builder.append(label, s);
					}

					void operator()(const tm& t) const {
						builder.appendTime(label, t);
					}

					void operator()(const repo::lib::RepoUUID& u) const {
						builder.appendUUID(label, u); // Use the explicit version becaues the specialisation is declared later
					}

				private:
					RepoBSONBuilder& builder;
					std::string label;
				};
			};

			// Template specialization
			template<> REPO_API_EXPORT void RepoBSONBuilder::append<repo::lib::RepoUUID>(
				const std::string &label,
				const repo::lib::RepoUUID &uuid
			);

			template<> REPO_API_EXPORT void RepoBSONBuilder::append<repo::lib::RepoVector3D>(
				const std::string &label,
				const repo::lib::RepoVector3D &vec
			);

			template<> REPO_API_EXPORT void RepoBSONBuilder::append<repo::lib::RepoMatrix>(
				const std::string &label,
				const repo::lib::RepoMatrix &mat
			);

			template<> REPO_API_EXPORT void RepoBSONBuilder::append<tm>(
				const std::string& label,
				const tm& mat
			);

			template<> REPO_API_EXPORT void RepoBSONBuilder::append<RepoBSON>(
				const std::string& label,
				const RepoBSON& obj
			);

		}// end namespace model
	} // end namespace core
} // end namespace repo
