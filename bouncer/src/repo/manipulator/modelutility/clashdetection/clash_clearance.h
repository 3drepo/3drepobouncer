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
					};

					Clearance(DatabasePtr handler, const ClashDetectionConfig& config)
						: Pipeline(handler, config), tolerance(config.tolerance)
					{
					}

					std::unique_ptr<Broadphase> createBroadphase() const override;

					std::unique_ptr<Narrowphase> createNarrowphase() const override;

					CompositeClash* createCompositeClash() override
					{
						return new ClearanceClash();
					}

					virtual void append(CompositeClash& c, const Narrowphase& r) const;

					virtual void createClashReport(const OrderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const
					{
						result.idA = objects.a;
						result.idB = objects.b;
						result.positions = {
							static_cast<const ClearanceClash&>(clash).line.start,
							static_cast<const ClearanceClash&>(clash).line.end
						};
						result.fingerprint = 0;
					}

				protected:
					double tolerance;
				};
			}
		}
	}
}