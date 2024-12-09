/**
*  Copyright (C) 2018 3D Repo Ltd
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
*  Assets BSON
*/

#pragma once

#include "repo/core/model/repo_model_global.h"
#include "repo/lib/datastructure/repo_vector.h"
#include "repo/lib/datastructure/repo_uuid.h"

namespace repo {
	namespace core {
		namespace model {
			class RepoBSON;	// Forward declaration of RepoBSON for operator

			class REPO_API_EXPORT RepoSupermeshMetadata
			{
			public:
				size_t numVertices;
				size_t numFaces;
				size_t numUVChannels;
				size_t primitive;
				repo::lib::RepoVector3D min;
				repo::lib::RepoVector3D max;
			};

			class REPO_API_EXPORT RepoAssets
			{
			public:

				RepoAssets();

				RepoAssets(
					const repo::lib::RepoUUID& revisionId,
					const std::string& database,
					const std::string& model,
					const repo::lib::RepoVector3D64& offset,
					const std::vector<std::string>& repoBundleFiles,
					const std::vector<std::string>& repoJsonFiles,
					const std::vector<RepoSupermeshMetadata>& metadata);

				~RepoAssets() {}

				operator RepoBSON() const;

				const std::vector<std::string> getBundleFiles() const
				{
					return repoBundleFiles;
				}

				const std::vector<std::string> getJsonFiles() const
				{
					return repoJsonFiles;
				}

				bool isEmpty() const;

			private:
				repo::lib::RepoUUID revisionId;
				std::string database;
				std::string model;
				repo::lib::RepoVector3D64 offset;
				std::vector<std::string> repoBundleFiles;
				std::vector<std::string> repoJsonFiles;
				std::vector<RepoSupermeshMetadata> metadata;
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
