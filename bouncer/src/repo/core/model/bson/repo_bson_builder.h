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

#include <string>
#include <mongo/bson/bson.h>
#include "../../../lib/datastructure/repo_matrix.h"
#include "../../../lib/datastructure/repo_uuid.h"
#include "repo_bson.h"

#include <boost/variant/static_visitor.hpp>
#include "repo/lib/datastructure/repo_variant.h"

#include <ctime>

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT RepoBSONBuilder : private mongo::BSONObjBuilder
			{
			public:
				RepoBSONBuilder();
				~RepoBSONBuilder();

				/**
				* Append a vector as object into the bson
				* This function creates an embedded RepoBSON and append that object as an array into the builder
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
					mongo::BSONObjBuilder::appendArray(label, array.obj());
				}

				void appendArray(
					const std::string &label,
					const RepoBSON &bson)
				{
					mongo::BSONObjBuilder::appendArray(label, bson);
				}

				
				void appendRepoVariant(
					const std::string& label,
					const repo::lib::RepoVariant& item);


				template<class T>
				void append(
					const std::string& label,
					const T& item)
				{
					mongo::BSONObjBuilder::append(label, item);
				}

				/**
				* Append a list of pairs into an arraybson of objects
				* @param label label to append the array against
				* @param list list of pairs to append
				* @param fstLabel label for #1 in the pair
				* @param sndLabel label for #2 in the pair
				*/
				void appendArrayPair(
					const std::string &label,
					const std::list<std::pair<std::string, std::string> > &list,
					const std::string &fstLabel,
					const std::string &sndLabel
					);

				/*!
				* Appends a pointer to some memory as binary mongo::BinDataGeneral type array.
				* Appends given data as a binary data blob into a given builder. Also
				* appends the count of the elements and the byte count of the array if
				* labels are specified
				* @param label Label for this element
				* @param data the data itself
				* @param byteCount size of data in bytes
				* @param byteCountLabel label to store byteCount
				* @param countLabel count label to store count
				*/
				template <class T>
				void appendBinary(
					const std::string &label,
					const T           *data,
					const uint32_t  &byteCount)
				{
					if (data && 0 < byteCount)
					{
						// Store data as a binary blob
						try{
							appendBinData(
								label, byteCount, mongo::BinDataGeneral,
								(void *)data);
						}
						catch (std::exception &e)
						{
							repoError << "Failed: " << e.what();
							exit(-1);
						}
					}
					else
					{
						repoWarning << "Trying to append a binary of size 0 into a bson. Skipping..";
					}
				}

				void appendElements(RepoBSON bson) {
					mongo::BSONObjBuilder::appendElements(bson);
				}

				void appendElementsUnique(RepoBSON bson) {
					mongo::BSONObjBuilder::appendElementsUnique(bson);
				}

				void appendTimeStamp(std::string label){
					appendTime(label, time(NULL) * 1000);
				}

				void appendTime(std::string label, const int64_t &ts){
					mongo::Date_t date = mongo::Date_t(ts);
					mongo::BSONObjBuilder::append(label, date);
				}
				
				void appendTime(std::string label, const tm& t) {
					tm tmCpy = t; // Copy because mktime can alter the struct
					int64_t time = static_cast<int64_t>(mktime(&tmCpy));

					// Check for a unsuccessful conversion
					if (time == -1)
					{
						repoError << "Failed converting date to mongo compatible format. tm malformed or date pre 1970?";
						exit(-1);
					}

					// Convert from seconds to milliseconds
					time = time * 1000;

					// Append time
					appendTime(label, time);
				}

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

				mongo::BSONObj mongoObj() { return mongo::BSONObjBuilder::obj();  }

				/**
				* Builds the BSON object as a temporary instance. This allows the caller to
				* peak at the contents of the builder, but the object will become invalid
				* as soon as the builder is modified or released. These temporary objects do
				* not contain the bin mappings.
				*/
				RepoBSON tempObj()
				{
					return RepoBSON(mongo::BSONObjBuilder::asTempObj());
				}

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
						// Filter out empty strings
						if (s != "")
							builder.append(label, s);
					}

					void operator()(const tm& t) const {
						builder.appendTime(label, t);
					}

				private:
					RepoBSONBuilder& builder;
					std::string label;
				};
			};


			// Template specialization
			template<> REPO_API_EXPORT void RepoBSONBuilder::append < repo::lib::RepoUUID > (
				const std::string &label,
				const repo::lib::RepoUUID &uuid
			);

			template<> REPO_API_EXPORT void RepoBSONBuilder::append < repo::lib::RepoVector3D > (
				const std::string &label,
				const repo::lib::RepoVector3D &vec
			);

			template<> REPO_API_EXPORT void RepoBSONBuilder::append < repo::lib::RepoMatrix > (
				const std::string &label,
				const repo::lib::RepoMatrix &mat
			);
		}// end namespace model
	} // end namespace core
} // end namespace repo
