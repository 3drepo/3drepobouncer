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

// ---
// The following are only for writeTicketImportJson
#define RAPIDJSON_HAS_STDSTRING 1
#include "repo/manipulator/modelutility/rapidjson/rapidjson.h"
#include "repo/manipulator/modelutility/rapidjson/document.h"
#include "repo/manipulator/modelutility/rapidjson/writer.h"
#include "repo/manipulator/modelutility/rapidjson/stringbuffer.h"
#include <fstream>
//-----

using namespace repo::lib;
using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

#pragma optimize("", off)

ClashDetectionReport ClashDetectionEngine::runClashDetection
	(const ClashDetectionConfig& config)
{
	clash::Pipeline* pipeline = nullptr;
	switch (config.type) {
	case ClashDetectionType::Clearance:
		pipeline = new clash::Clearance(handler, config);
		break;
	case ClashDetectionType::Hard:
		pipeline = new clash::Hard(handler, config);
		break;
	default:
		throw std::invalid_argument("Unknown clash detection type");
	}

	auto results = pipeline->runPipeline();

	return results;
}

ClashDetectionEngine::ClashDetectionEngine(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler)
	:handler(handler)
{
}

void ClashDetectionEngine::writeClashDetectionResultsJson(const ClashDetectionReport& report, const ClashDetectionConfig& config)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

	writer.StartObject();

	writer.Key("config");
	writer.String(config.path);

	writer.Key("clashes");

	writer.StartArray();

	for (const auto& clash : report.clashes)
	{
		writer.StartObject();
		writer.Key("a");
		writer.String(clash.idA.toString());
		writer.Key("b");
		writer.String(clash.idB.toString());
		writer.EndObject();
	}

	writer.EndArray();

	writer.EndObject();

	std::ofstream outFile("C:\\3drepo\\3drepobouncer_ISSUE797\\results.json", std::ios::out | std::ios::trunc);
	if (!outFile.is_open())
	{
		throw std::ios_base::failure("Failed to open file");
	}
	outFile << buffer.GetString();
	outFile.close();
}