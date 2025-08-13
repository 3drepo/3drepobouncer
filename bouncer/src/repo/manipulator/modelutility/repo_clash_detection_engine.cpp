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

#include "repo_clash_detection_engine.h"

#include "clashdetection/clash_hard.h"
#include "clashdetection/clash_clearance.h"

using namespace repo::lib;
using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

ClashDetectionReport ClashDetectionEngine::runClashDetection
	(const ClashDetectionConfig& config)
{
	clash::Pipeline* pipeline = nullptr;
	switch (config.type) {
	case ClashDetectionType::Clearance:
		pipeline = new clash::Clearance(handler, config);
	case ClashDetectionType::Hard:
		pipeline = new clash::Hard(handler, config);
	default:
		throw std::invalid_argument("Unknown clash detection type");
	}

	return pipeline->runPipeline();
}

ClashDetectionEngine::ClashDetectionEngine(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler)
	:handler(handler)
{
}