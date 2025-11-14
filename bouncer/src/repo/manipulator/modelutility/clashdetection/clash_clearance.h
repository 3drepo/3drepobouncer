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

#include "clash_pipelines.h"
#include "repo/lib/datastructure/repo_uuid.h"
#include "repo/lib/datastructure/repo_line.h"


namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				struct Pair
				{
					// This struct will hold the results of the clash detection.
					// It can be extended with more fields as needed.
					std::vector<repo::lib::RepoUUID> clashes;
				};

				class Clearance : public Pipeline
				{
				public:

					class ClearanceClash : public CompositeClash
					{
					public:
						repo::lib::RepoLine line;

						ClearanceClash()
							:line(repo::lib::RepoLine::Max())
						{
						}

						void append(const repo::lib::RepoLine& otherLine);
					};

					Clearance(DatabasePtr handler, const ClashDetectionConfig& config)
						: Pipeline(handler, config), tolerance(config.tolerance)
					{
					}

					void run(const Graph& graphA, const Graph& graphB) override;

					void createClashReport(const OrderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const;

				protected:
					double tolerance;
				};
			}
		}
	}
}