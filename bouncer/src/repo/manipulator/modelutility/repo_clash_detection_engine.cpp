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
	std::unique_ptr<clash::Pipeline> pipeline;
	switch (config.type) {
	case ClashDetectionType::Clearance:
		pipeline = std::make_unique<clash::Clearance>(handler, config);
		break;
	case ClashDetectionType::Hard:
		pipeline = std::make_unique<clash::Hard>(handler, config);
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

ClashDetectionEngine::ClashDetectionEngine(
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler)
	:handler(handler)
{
}

void ClashDetectionEngineUtils::writeJson(const ClashDetectionReport& report,
	std::basic_ostream<char, std::char_traits<char>>& stream)
{
	rapidjson::OStreamWrapper osw(stream);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);

	writer.StartObject();

	writer.Key("clashes");

	writer.StartArray();
	if (!report.errors.size()) { // Don't bother to write loads of clash data if there were errors
		for (const auto& clash : report.clashes)
		{
			writer.StartObject();
			writer.Key("a");
			writer.String(clash.idA);
			writer.Key("b");
			writer.String(clash.idB);
			writer.Key("positions");
			writer.StartArray();
			for (const auto& position : clash.positions)
			{
				writer.StartArray();
				writer.Double(position.x);
				writer.Double(position.y);
				writer.Double(position.z);
				writer.EndArray();
			}
			writer.EndArray();
			writer.Key("fingerprint");
			writer.String(std::to_string(clash.fingerprint));
			writer.EndObject();
		}
	}
	writer.EndArray();

	if (report.errors.size()) {
		writer.Key("errors");
		writer.StartArray();
		for (auto& error : report.errors) {
			auto json = error->toJson();
			writer.RawValue(json.c_str(), json.size(), rapidjson::kObjectType);
		}
		writer.EndArray();
	}

	writer.EndObject();
}

void ClashDetectionEngineUtils::writeJson(const ClashDetectionReport& report,
	const ClashDetectionConfig& config)
{
	std::ofstream outFile(config.resultsFile, std::ios::out | std::ios::trunc);
	if (!outFile.is_open())
	{
		throw std::ios_base::failure("Failed to open file");
	}
	writeJson(report, outFile);
	outFile.close();
}
