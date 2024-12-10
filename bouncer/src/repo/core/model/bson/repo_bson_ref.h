/**
*  Copyright (C) 2019 3D Repo Ltd
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
*  Ref BSON
*/

#pragma once

#include "repo/repo_bouncer_global.h"
#include "repo/lib/datastructure/repo_variant.h"
#include <unordered_map>

namespace repo {
	namespace core {
		namespace model {

			class RepoBSON;
			//------------------------------------------------------------------------------
			//
			// Fields specific to ref only
			//
			//------------------------------------------------------------------------------
#define REPO_REF_LABEL_LINK "link"
#define REPO_REF_LABEL_SIZE "size"
#define REPO_REF_LABEL_TYPE "type"


			class REPO_API_EXPORT RepoRef
			{
			public:
				const static std::string REPO_REF_TYPE_FS;
				const static std::string REPO_REF_TYPE_GRIDFS;
				const static std::string REPO_REF_TYPE_UNKNOWN;

				using Metadata = std::unordered_map<std::string, repo::lib::RepoVariant>;

				enum class RefType {
					FS,
					UNKNOWN,
				};

				// Only FS type is currently supported
				RepoRef(
					const std::string& link,
					const size_t& size,
					const Metadata& metadata = {}
				);

				RepoRef(RepoBSON bson);

				~RepoRef() {}

				operator RepoBSON() const;

				static std::string convertTypeAsString(const RefType &type);

				std::string getFileName() const;

				std::string getRefLink() const;

				size_t getFileSize() const;

				RefType getType() const;

			protected:

				virtual void serialise(class RepoBSONBuilder& builder) const;

			private:

				RefType type;
				std::string link;
				size_t size;
				std::string name;
				std::unordered_map<std::string, repo::lib::RepoVariant> metadata; //Serialise only
			};

			template<class IdType>
			class REPO_API_EXPORT RepoRefT : public RepoRef
			{
			public:

				RepoRefT(
					const IdType& id,
					const std::string& link,
					const size_t& size,
					const Metadata& metadata = {}
				);

				RepoRefT(RepoBSON bson);

				IdType getId() const;

			protected:

				void serialise(class RepoBSONBuilder& builder) const;

			private:
				IdType id;
			};

		}// end namespace model
	} // end namespace core
} // end namespace repo
