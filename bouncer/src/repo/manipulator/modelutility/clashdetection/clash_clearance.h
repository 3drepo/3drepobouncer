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
#include "geometry_tests.h"

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

					class Result : public NarrowphaseResult
					{
					public:
						repo::lib::RepoLine line;
					};

					Clearance(DatabasePtr handler, const ClashDetectionConfig& config)
						: Pipeline(handler, config), tolerance(config.tolerance)
					{
					}

					void broadphase(const Bvh& a, const Bvh& b, BroadphaseResults& results) const override
					{
						// The clearance clash detection does not require a broadphase step.
						// It is assumed that the input BVH is already filtered to only include relevant nodes.
						// Therefore, we can skip this step and directly proceed to the narrowphase.

						throw new std::exception("Not Implemented Yet");
					}

					bool narrowphase(const repo::lib::RepoTriangle& a, const repo::lib::RepoTriangle& b, NarrowphaseResult& c) const override
					{
						auto& result = static_cast<Result&>(c);
						result.line = geometry::closestPointTriangleTriangle(a, b);
						return result.line.magnitude() < tolerance;
					}

					std::unique_ptr<NarrowphaseResult> createNarrowphaseResult() override
					{
						return std::make_unique<Result>();
					}

					CompositeClash* createCompositeClash() override
					{
						return new ClearanceClash();
					}

					virtual void append(CompositeClash& c, const NarrowphaseResult& r) const
					{
						auto& clash = static_cast<ClearanceClash&>(c);
						auto& result = static_cast<const Result&>(r);
						if (result.line.magnitude() < clash.line.magnitude()) {
							clash.line = result.line;
						}
					}

					virtual void createClashReport(const UnorderedPair& objects, const CompositeClash& clash, ClashDetectionResult& result) const
					{
						throw std::exception("Not Implemented Yet");
					}

				protected:
					double tolerance;
				};
			}
		}
	}
}