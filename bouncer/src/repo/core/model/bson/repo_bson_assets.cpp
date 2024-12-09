/**
*  Copyright (C) 2024 3D Repo Ltd
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

#include "repo_bson_assets.h"
#include "repo_bson_builder.h"
#include "repo/lib/datastructure/repo_uuid.h"

using namespace repo::core::model;

RepoAssets::RepoAssets()
	:revisionId(repo::lib::RepoUUID::defaultValue)
{
}

RepoAssets::RepoAssets(
	const repo::lib::RepoUUID& revisionId,
	const std::string& database,
	const std::string& model,
	const repo::lib::RepoVector3D64& offset,
	const std::vector<std::string>& repoBundleFiles,
	const std::vector<std::string>& repoJsonFiles,
	const std::vector<RepoSupermeshMetadata>& metadata
)
	:revisionId(revisionId),
	database(database),
	model(model),
	offset(offset),
	repoBundleFiles(repoBundleFiles),
	repoJsonFiles(repoJsonFiles),
	metadata(metadata)
{
}

RepoAssets::operator RepoBSON() const
{
	RepoBSONBuilder builder;

	builder.append(REPO_LABEL_ID, revisionId);

	if (repoBundleFiles.size())
		builder.appendArray(REPO_ASSETS_LABEL_ASSETS, repoBundleFiles);

	if (!database.empty())
		builder.append(REPO_LABEL_DATABASE, database);

	if (!model.empty())
		builder.append(REPO_LABEL_MODEL, model);

	if (repoJsonFiles.size())
		builder.appendArray(REPO_ASSETS_LABEL_JSONFILES, repoJsonFiles);

	builder.appendArray(REPO_ASSETS_LABEL_OFFSET, offset.toStdVector());

	// Metadata is provided in an array with the same indexing as asset names.
	// The metadata schema uses the object (instead of array) encoding of
	// vectors, so that the type & entire array is a fixed size and cam be
	// pre-allocated.

	std::vector<RepoBSON> metadataNodes;
	for (auto& meta : metadata)
	{
		RepoBSONBuilder metadataBuilder;
		metadataBuilder.append(REPO_ASSETS_LABEL_NUMVERTICES, (unsigned int)meta.numVertices);
		metadataBuilder.append(REPO_ASSETS_LABEL_NUMFACES, (unsigned int)meta.numFaces);
		metadataBuilder.append(REPO_ASSETS_LABEL_NUMUVCHANNELS, (unsigned int)meta.numUVChannels);
		metadataBuilder.append(REPO_ASSETS_LABEL_PRIMITIVE, (unsigned int)meta.primitive);
		metadataBuilder.appendVector3DObject(REPO_ASSETS_LABEL_MIN, meta.min);
		metadataBuilder.appendVector3DObject(REPO_ASSETS_LABEL_MAX, meta.max);
		metadataNodes.push_back(metadataBuilder.obj());
	}

	if (metadataNodes.size())
		builder.appendArray(REPO_ASSETS_LABEL_METADATA, metadataNodes);

	return builder.obj();
}

bool RepoAssets::isEmpty() const
{
	return repoBundleFiles.empty() && repoJsonFiles.empty() && revisionId.isDefaultValue() && database.empty() && model.empty();
}