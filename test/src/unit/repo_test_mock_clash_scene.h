/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include <repo/manipulator/modelutility/repo_clash_detection_config.h>
#include <repo/lib/datastructure/repo_container.h>
#include <repo/lib/datastructure/repo_uuid.h>
#include <repo/lib/datastructure/repo_matrix.h>
#include <repo/lib/datastructure/repo_line.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <vector>

#include "repo_test_random_generator.h"

namespace testing {

	using TransformLine = std::pair<repo::lib::RepoLine, repo::lib::RepoMatrix>;
	using TransformLines = std::pair<TransformLine, TransformLine>;
	using Lines = std::pair<repo::lib::RepoLine, repo::lib::RepoLine>;
	using UUIDPair = std::pair<repo::lib::RepoUUID, repo::lib::RepoUUID>;

	struct ClashDetectionConfigHelper : public repo::manipulator::modelutility::ClashDetectionConfig
	{
		ClashDetectionConfigHelper();

		void addCompositeObjects(
			const repo::lib::RepoUUID& uniqueIdA, 
			const repo::lib::RepoUUID& uniqueIdB
		);
	};

	struct ClashGenerator
	{
		RepoRandomGenerator random;

		// Creates a pair of lines of a given length, that are separated by the given
		// distance exactly, offset from the origin by distance offset.

		Lines createLines(
			double length,
			double distance,
			double offset
		);

		// Creates a pair of lines that are separated by the given distance exactly when
		// transformed by their respective matrices. The magnitude of the offset of the
		// matrix will be on the order of offset.

		TransformLines createLinesTransformed(
			double length,
			double distance,
			double offset
		);
	};

	struct MockClashScene
	{
		std::vector<repo::core::model::RepoBSON> bsons;
		repo::lib::RepoUUID root;

		MockClashScene(const repo::lib::RepoUUID& revision);

		const repo::core::model::MeshNode add(repo::lib::RepoLine line, const repo::lib::RepoUUID& parentSharedId);

		const repo::core::model::TransformationNode add(const repo::lib::RepoMatrix& matrix);

		UUIDPair add(TransformLines lines, ClashDetectionConfigHelper& config);
	};
}