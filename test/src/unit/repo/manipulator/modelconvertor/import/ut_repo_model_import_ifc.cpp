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
#include <gmock/gmock.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_ifc.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_manager.h>
#include <repo_log.h>
#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_database_info.h"
#include "../../../../repo_test_scene_utils.h"
#include "../../../../repo_test_matchers.h"

using namespace repo::manipulator::modelconvertor;
using namespace repo::core::model;
using namespace testing;

#define TESTDB "IFCModelImportTest"

namespace IfcModelImportUtils
{
	repo::core::model::RepoScene* ModelImportManagerImport(std::string collection, std::string filename)
	{
		ModelImportConfig config(
			true,
			ModelUnits::MILLIMETRES,
			"",
			0,
			repo::lib::RepoUUID::createUUID(),
			TESTDB,
			collection);

		auto handler = getHandler();

		uint8_t err;
		std::string msg;

		ModelImportManager manager;
		auto scene = manager.ImportFromFile(filename, config, handler, err);
		scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());
		scene->loadScene(handler.get(), msg);

		return scene;
	}
}

TEST(IFCModelImport, RelContainedInSpatialStructure)
{
	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse1));
	SceneUtils utils(scene);

	// This test file contains multiple instances of a Tree (P0..n), assigned to
	// different parts using IfcRelContainedInSpatialStructure.

	// P0 (building element proxy with representation) should sit under Level 0,
	// under the grouping of IfcBuildingElementProxy

	auto node = utils.findNodeByMetadata("Name", "P0");
	EXPECT_THAT(node.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->IfcBuildingElementProxy->P0"));
	EXPECT_THAT(node.isLeaf(), IsTrue());

	// P1 sits under IfcSpace, itself under Level 0. Only the first container
	// (IfcSpace) should have a grouping in the tree.

	node = utils.findNodeByMetadata("Name", "P1");
	EXPECT_THAT(node.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->Space1->IfcBuildingElementProxy->P1"));
	EXPECT_THAT(node.isLeaf(), IsTrue());

	// Spatial elements may be multiple levels deep.

	node = utils.findNodeByMetadata("Name", "P5");
	EXPECT_THAT(node.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->P3->P4->IfcBeam->P5"));
	EXPECT_THAT(node.isLeaf(), IsTrue());	
}

TEST(IFCModelImport, RelContainedInSpatialStructure_DoublyPlacedObjects)
{
	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse_doublyPlacedObjects));
	SceneUtils utils(scene);

	// If an entity combines RelContainedInSpatialStructure with Aggregations or
	// Nesting, it is placed twice. This is not supported by all tools. We will
	// import the file, with RelContainedInSpatialStructure always taking priority.
	
	std::vector<std::string> beams = { "Beam1", "Beam2" };
	for (auto name : beams)
	{
		auto node = utils.findNodeByMetadata("Name", name);
		EXPECT_THAT(node.getPath(), HasSubstr("Level0"));
		EXPECT_THAT(node.isLeaf(), IsTrue());
	}
}

TEST(IFCModelImport, RelAssigns)
{
	// IfcRelAssigns is used to infer logical relationships such as client->supplier
	// or navigation (room1->room2). For example, a Lift Shaft may be contained
	// under Level 0, but assigned to multiple levels above it.

	// simpleHouse1 contains an IfcBuildingElementProxy that is assigned to Level0,
	// but has no other relationships.

	// IfcRelAssigns should not affect either the tree or metadata in the current
	// version.

	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse1));
	SceneUtils utils(scene);

	auto node = utils.findNodeByMetadata("Name", "P2");

	EXPECT_THAT(node.isLeaf(), IsTrue());
	EXPECT_THAT(node.getPath(), Eq("rootNode->P2"));
	EXPECT_THAT(node.getMetadata().size(), Eq(3));
}

TEST(IFCModelImport, RelDecomposes)
{
	// Decomposes is used to build the hierarchy. We support nesting and aggregation.
	// In theory we should support Projects and Voids too, but we don't import these
	// geometric elements (this test specifically includes those).

	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse1));
	SceneUtils utils(scene);

	// IfcRelAggregates and IfcRelNests should result in the same structure

	auto beam3 = utils.findNodeByMetadata("Name", "Beam1");
	EXPECT_THAT(beam3.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->IfcWall->Wall1->Door1->Beam1"));

	beam3 = utils.findNodeByMetadata("Name", "Beam2");
	EXPECT_THAT(beam3.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->IfcWall->Wall2->Door2->Beam2"));

	// IfcRelProjectsElement does not imply spatial containment, so never contributes
	// to the tree, even if no other relationships exist.

	// IfcRelVoidsElement also does not contribute to the tree.
}

TEST(IFCModelImport, RelAssociatesClassification)
{
	// IfcRelAssociatesClassification should insert classification data as metadata.
	// Other IfcRelAssociates relationships are ignored.

	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse_classification));
	SceneUtils utils(scene);

	std::vector<std::string> classification1 = { "P0", "P1", "P2" }; // These objects have different levels of indirection to the IfcClassification
	std::vector<std::string> classification2 = { "P3" };

	for (auto name : classification1)
	{
		auto node = utils.findNodeByMetadata("Name", name);
		auto metadata = node.getMetadata();
		EXPECT_THAT(metadata["Classification1::Source"], Vq("3DRepo"));
		EXPECT_THAT(metadata["Classification1::Edition"], Vq("Unit Tests"));
		EXPECT_THAT(metadata["Classification1::EditionDate"], Vq("2025-01-21"));
		EXPECT_THAT(metadata["Classification1::Name"], Vq("Classification1"));
		EXPECT_THAT(metadata["Classification1::Description"], Vq("Unit Test Classification 1"));
		EXPECT_THAT(metadata["Classification1::Specification"], Vq("3drepo.io"));
		EXPECT_THAT(metadata["Classification1::ReferenceTokens"], Vq("(ReferenceToken1)"));
	}

	for (auto name : classification2)
	{
		auto node = utils.findNodeByMetadata("Name", name);
		auto metadata = node.getMetadata();
		EXPECT_THAT(metadata["ExternalClassificationReference::Description"], Vq("Reference 3"));
		EXPECT_THAT(metadata["ExternalClassificationReference::Identification"], Vq("myUUID"));
		EXPECT_THAT(metadata["ExternalClassificationReference::Location"], Vq("3drepo.io/classification"));
		EXPECT_THAT(metadata["ExternalClassificationReference::Name"], Vq("ExternalClassificationReference"));
		EXPECT_THAT(metadata["ExternalClassificationReference::Sort"], Vq("TestSortIdentifier"));
	}
}

TEST(IFCModelImport, RelConnects)
{
	// RelConnects are ignored for now.
}

TEST(IFCModelImport, RelDeclares)
{
	// We do not currently process IfcRelDeclares - we assume only one IfcProject
	// exists. IFCOS should implicitly consider it when iterating the geometry
	// however.
}

TEST(IFCModelImport, RelDefinesByType)
{
	// IfcRelDefinesByType is used to declare an IfcTypeObject as a superclass of
	// an IfcObject; the entity should inhert (and can override) the type object's
	// properties. This relationship does not affect the tree.
}

TEST(IFCModelImport, RelDefinesByObject)
{
	// IfcRelDefinesByObject allows an entity to inhert the properties of another
	// object, like a superclass. Related objects take the relating objects
	// property sets and represetnations. This behaves similarly to DefinesByType,
	// except that in this case the super-object is an entity, rather than a
	// dedicated type object.
}

TEST(IFCModelImport, RelDefinesByProperties)
{
	// IfcRelDefinesByProperties is how property sets are associated with
	// instances.
}

TEST(IFCModelImport, RelDefinesByTemplate)
{
	// IfcRelDefinesByTemplate allows a template of a property set to be declared
	// for multiple property set definitions. The template may include things like
	// the name, description and possibly contained properties. The relationship
	// is hierarchical in the object oriented sense. This does not affect the tree.
}

TEST(IFCModelImport, PropertySets)
{
	// Check that property set definitions and templates work...




}

TEST(IFCModelImport, PredefinedPropertySets)
{

}

TEST(IFCModelImport, ComplexProperties)
{
	// GIS and dates
}

TEST(IFCModelImport, InbuiltUnits)
{
	// Check units on the root attributes of the types
}

TEST(IFCModelImport, OverrideUnits)
{

}

TEST(IFCModelImport, ImperialUnits)
{

}

TEST(IFCModelImport, Unicode)
{

}

/*
TEST(IfcModelImport, Unicode)
{
	uint8_t errMsg;

	auto importer = RepoModelImportUtils::ImportIFC(getDataPath("duct.ifc"), errMsg);
	auto scene = importer->generateRepoScene(errMsg);

	EXPECT_THAT(scene, IsTrue());

	// Check the metadata entries to ensure special units use proper unicode
	// encoding
	// (Note if this test breaks it is most likely a compiler flags issue)

	auto metadata = scene->getAllMetadata(repo::core::model::RepoScene::GraphType::DEFAULT);
	for (auto n : metadata)
	{
		auto mn = dynamic_cast<repo::core::model::MetadataNode*>(n);
		if (mn->getName() == "Oval Duct:Standard:329435")
		{
			auto md = mn->getAllMetadata();
			for (auto& m : md)
			{
				if (m.first.starts_with("Mechanical::Area"))
				{
					// The full key is Mechanical::Area (m²), and the last bytes
					// should be {0x28, 0x6d, 0xc2, 0xb2, 0x29} (the superscript
					// is encoded as 0xc2, 0xb2 in unicode.

					const uint8_t s[] = { 0x28, 0x6d, 0xc2, 0xb2, 0x29 };

					EXPECT_THAT(m.first.length(), Eq(22));
					EXPECT_THAT(memcmp(m.first.c_str() + m.first.size() - 5, s, 5), Eq(0));
				}
			}
		}
	}
}
*/
