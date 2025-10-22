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
#include "clashdetection/clash_exceptions.h"

#define RAPIDJSON_HAS_STDSTRING 1
#include "repo/manipulator/modelutility/rapidjson/rapidjson.h"
#include "repo/manipulator/modelutility/rapidjson/document.h"
#include "repo/manipulator/modelutility/rapidjson/writer.h"
#include "repo/manipulator/modelutility/rapidjson/ostreamwrapper.h"
#include <fstream>

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
		break;
	case ClashDetectionType::Hard:
		pipeline = new clash::Hard(handler, config);
		break;
	default:
		throw std::invalid_argument("Unknown clash detection type");
	}

	ClashDetectionReport results;

	try {
		results = pipeline->runPipeline();
	}
	catch (const clash::ClashDetectionException& e) {
		
		// These are runtime exceptions that indicate a logical or validation
		// error that is not to do with the state of the process. These should
		// be indicated to the user via the report.

		results.errors.push_back(std::move(e.clone()));
	}

	return results;
}

ClashDetectionEngine::ClashDetectionEngine(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler)
	:handler(handler)
{
}

void ClashDetectionEngineUtils::writeJson(const ClashDetectionReport& report, std::basic_ostream<char, std::char_traits<char>>& stream)
{
	// Currently we only support one output format, which is Json to a location
	// defined by the config.

	rapidjson::OStreamWrapper osw(stream);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);

	writer.StartObject();

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

	if (report.errors.size()) {
		writer.Key("errors");
		writer.StartArray();

		for (auto& error : report.errors)
		{
			writer.StartObject();

			if (dynamic_cast<clash::MeshBoundsException*>(error.get()) != nullptr)
			{
				writer.Key("type");
				writer.String("meshBounds");
			}
			else if (dynamic_cast<clash::TransformBoundsException*>(error.get()) != nullptr)
			{
				writer.Key("type");
				writer.String("transformBounds");
			}
			else {
				writer.Key("type");
				writer.String("unknownError");
			}

			writer.EndObject();
		}

		writer.EndArray();
	}

	writer.EndObject();
}

void ClashDetectionEngineUtils::writeJson(const ClashDetectionReport& report, const ClashDetectionConfig& config)
{
	std::ofstream outFile(config.resultsFile, std::ios::out | std::ios::trunc);
	if (!outFile.is_open())
	{
		throw std::ios_base::failure("Failed to open file");
	}
	writeJson(report, outFile);
	outFile.close();
}
