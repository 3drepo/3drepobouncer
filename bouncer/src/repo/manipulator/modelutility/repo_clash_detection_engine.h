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

#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include <repo/repo_bouncer_global.h>
#include <repo/lib/datastructure/repo_vector.h>
#include <repo/manipulator/modelutility/repo_clash_detection_config_fwd.h>
#include <repo/manipulator/modelutility/clashdetection/clash_exceptions.h>
#include <repo/core/handler/repo_database_handler_abstract.h>

namespace repo {
	namespace manipulator {
		namespace modelutility {

			struct ClashDetectionResult
			{
				// The IDs of the two Composite Objects involved in the clash.
				// Composite Objects are the lowest level of granularity reported - if
				// it is desirable to know the details of the clashes between two
				// MeshNodes, they should be in separate Composite Objects.

				repo::lib::RepoUUID idA;
				repo::lib::RepoUUID idB;

				// The positions in project coordinates that represent the clash in a
				// high level way. The exact meaning of these depends on the clash test.
				// For example, in a clearance clash, these will be the single two
				// closest points.

				std::vector<repo::lib::RepoVector3D64> positions;

				// Unique identifier of this clash. If the same objects clash in the
				// exact same configuration, the fingerprint will be the same.
				// The fingerprint does not include the composite ids, only the geometry.
				// In this way the Id pair & fingerprint can be used to determine if a
				// clash has changed since the last run.

				size_t fingerprint;
			};

			struct ClashDetectionReport
			{
				std::vector<ClashDetectionResult> clashes;
				std::vector<std::shared_ptr<clash::ClashDetectionException>> errors;
			};

			REPO_API_EXPORT class ClashDetectionEngine
			{
			public:
				ClashDetectionEngine(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler);
				~ClashDetectionEngine() = default;

				ClashDetectionReport runClashDetection(const ClashDetectionConfig& config);

			protected:
				std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler;
			};

			class ClashDetectionEngineUtils {
			public:
				static void writeJson(const ClashDetectionReport& report, const ClashDetectionConfig& config);
				static void writeJson(const ClashDetectionReport& report, std::basic_ostream<char, std::char_traits<char>>& stream);
			};

		} // namespace modelutility
	} // namespace manipulator
}