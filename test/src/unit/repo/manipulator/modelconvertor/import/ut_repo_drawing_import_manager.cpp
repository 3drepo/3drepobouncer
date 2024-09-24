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

#include <gtest/gtest.h>
#include <repo/manipulator/modelconvertor/import/repo_drawing_import_manager.h>
#include <repo/lib/repo_log.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_database_info.h"
#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "boost/iostreams/stream.hpp"
#include "boost/iostreams/device/array.hpp"
#include "../../bouncer/src/repo/error_codes.h"

using namespace repo::manipulator::modelconvertor;

TEST(DrawingImportManager, ImportDGN)
{
	DrawingImportManager manager;
	DrawingImageInfo drawing;
	uint8_t error;
	manager.importFromFile(drawing, getDataPath(dgnDrawing), ".dgn", error);

	EXPECT_EQ(error, REPOERR_OK);
	EXPECT_GT(drawing.data.size(), 0);

	boost::iostreams::basic_array_source<char> svgArraySource((char*)drawing.data.data(), drawing.data.size());
	boost::iostreams::stream<boost::iostreams::basic_array_source<char>> svgStream(svgArraySource);

	boost::property_tree::ptree svgTree;
	boost::property_tree::read_xml(svgStream, svgTree); // This will throw an exception if the xml is not well formed

	// Check a number of properties that we expect to be consistent

	EXPECT_EQ(svgTree.get<std::string>("svg.<xmlattr>.width"), "1024");
	EXPECT_EQ(svgTree.get<std::string>("svg.<xmlattr>.height"), "768");
	EXPECT_EQ(svgTree.get<std::string>("svg.<xmlattr>.viewBox"), "0 0 1024 768");
}

TEST(DrawingImportManager, ImportDWG)
{
	DrawingImportManager manager;
	DrawingImageInfo drawing;
	uint8_t error;
	manager.importFromFile(drawing, getDataPath(dwgDrawing), ".dwg", error);

	EXPECT_EQ(error, REPOERR_OK);
	EXPECT_GT(drawing.data.size(), 0);

	boost::iostreams::basic_array_source<char> svgArraySource((char*)drawing.data.data(), drawing.data.size());
	boost::iostreams::stream<boost::iostreams::basic_array_source<char>> svgStream(svgArraySource);

	std::string s((char*)drawing.data.data(), drawing.data.size());
	std::cout << s << std::endl;

	boost::property_tree::ptree svgTree;
	boost::property_tree::read_xml(svgStream, svgTree); // This will throw an exception if the xml is not well formed

	// Check a number of properties that we expect to be consistent

	// We expect the dimensions and viewbox to be set, as these are required by the
	// frontend.

	EXPECT_EQ(svgTree.get<std::string>("svg.<xmlattr>.width"), "1024");
	EXPECT_EQ(svgTree.get<std::string>("svg.<xmlattr>.height"), "768");
	EXPECT_EQ(svgTree.get<std::string>("svg.<xmlattr>.viewBox"), "0 0 1024 768");


	int groupPrimitives = 0;
	for (auto g : svgTree.get_child("svg"))
	{
		// We expect that the white square in the test drawing is drawn as a black
		// square, because we set the background of the exporter to White.

		auto p = g.second.get_child_optional("polyline");
		if (p) {
			EXPECT_EQ(g.second.get<std::string>("<xmlattr>.stroke"), "rgb(0,0,0)");
			groupPrimitives++;
		}

		// We expect the Red circle to be off-shade, because we apply a colour
		// correction.

		auto c = g.second.get_child_optional("circle");
		if (c) {
			EXPECT_EQ(g.second.get<std::string>("<xmlattr>.stroke"), "rgb(127,0,0)");
			groupPrimitives++;
		}
	}

	// And of course expect that the DWG holds the above geometries

	EXPECT_EQ(groupPrimitives, 2);
}

