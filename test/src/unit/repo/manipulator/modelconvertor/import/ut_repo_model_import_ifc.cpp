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
#include <repo/lib/datastructure/repo_structs.h>
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

	/*
	* The tree is predominantly built by two relationship objects:
	* RelContainedInSpatialStructure for spatial relationhsips and RelDecomposes
	* for part/whole relationships.
	*
	* This test looks for instances of a tangible entity placed under spatial
	* entities at multiple levels, including where those spatial entities can
	* be multiple levels deep.
	*/

	auto node = utils.findNodeByMetadata("Name", "P0");
	EXPECT_THAT(node.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->IfcBuildingElementProxy->P0"));
	EXPECT_THAT(node.isLeaf(), IsTrue());

	node = utils.findNodeByMetadata("Name", "P1");
	EXPECT_THAT(node.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->Space1->IfcBuildingElementProxy->P1"));
	EXPECT_THAT(node.isLeaf(), IsTrue());

	// P3 and P4 are IfcSpaces; because the beam is the first tangible entity
	// encountered, the grouping is under P4, rather then the IfcBuildingStorey.

	node = utils.findNodeByMetadata("Name", "P5");
	EXPECT_THAT(node.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->P3->P4->IfcBeam->P5"));
	EXPECT_THAT(node.isLeaf(), IsTrue());
}

TEST(IFCModelImport, RelContainedInSpatialStructure_DoublyPlacedObjects)
{
	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse_doublyPlacedObjects));
	SceneUtils utils(scene);

	/*
	* If an entity combines RelContainedInSpatialStructure with Aggregations or
	* Nesting, it is placed twice. This is not supported by all tools. We will
	* import the file, with RelContainedInSpatialStructure always taking priority.
	*/

	std::vector<std::string> beams = { "Beam1", "Beam2" };
	for (auto name : beams)
	{
		auto node = utils.findNodeByMetadata("Name", name);
		EXPECT_THAT(node.getPath(), HasSubstr("Level 0"));
		EXPECT_THAT(node.isLeaf(), IsTrue());
	}
}

TEST(IFCModelImport, RelAssigns)
{
	/*
	* IfcRelAssigns is used to infer logical relationships such as client->supplier
	* or navigation (room1->room2). For example, a Lift Shaft may be contained
	* under Level 0, but assigned to multiple levels above it.
	*
	* IfcRelAssigns should not affect either the tree or metadata in the current
	* importer.
	*/

	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse1));
	SceneUtils utils(scene);

	auto node = utils.findNodeByMetadata("Name", "P2");

	EXPECT_THAT(node.isLeaf(), IsTrue());
	EXPECT_THAT(node.getPath(), Eq("rootNode->P2"));
	EXPECT_THAT(node.getMetadata().size(), Eq(3));
}

TEST(IFCModelImport, RelDecomposes)
{
	/*
	* IfcRelDecomposes represents part/whole relationships of tangible entities and
	* along with RelContainedInSpatialStructure is one of the primary ways in which
	* we build the tree.
	*
	* We support Nesting and Aggregation.
	*
	* IFCOS will use IfcRelVoids to build the geometry, but we do not represent
	* Openings the tree or collect metadata for them. The same is true for
	* IfcRelProjectsElements.
	*/

	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse1));
	SceneUtils utils(scene);

	// IfcRelAggregates and IfcRelNests should result in the same structure

	auto beam3 = utils.findNodeByMetadata("Name", "Beam1");
	EXPECT_THAT(beam3.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->IfcWall->Wall1->Door1->Beam1"));

	beam3 = utils.findNodeByMetadata("Name", "Beam2");
	EXPECT_THAT(beam3.getPath(), Eq("rootNode->Project Number->Default->IfcBuilding->Level 0->IfcWall->Wall2->Door2->Beam2"));
}

TEST(IFCModelImport, RelAssociatesClassification)
{
	/*
	* The importer will use IfcRelAssociatesClassification to build metadata,
	* but nothing else. Other IfcRelAssociates relationships are ignored.
	*/

	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse_classification));
	SceneUtils utils(scene);

	std::vector<std::string> classification1 = { "P0", "P1", "P2" }; // These objects have different levels of indirection to the IfcClassification
	std::vector<std::string> classification2 = { "P3" };

	for (auto name : classification1)
	{
		auto node = utils.findNodeByMetadata("Name", name);
		auto metadata = node.getMetadata();
		EXPECT_THAT(metadata["Classification1::Source"], Vs("3DRepo"));
		EXPECT_THAT(metadata["Classification1::Edition"], Vs("Unit Tests"));
		EXPECT_THAT(metadata["Classification1::EditionDate"], Vs("2025-01-21"));
		EXPECT_THAT(metadata["Classification1::Name"], Vs("Classification1"));
		EXPECT_THAT(metadata["Classification1::Description"], Vs("Unit Test Classification 1"));
		EXPECT_THAT(metadata["Classification1::Specification"], Vs("3drepo.io"));
		EXPECT_THAT(metadata["Classification1::ReferenceTokens"], Vs("(ReferenceToken1, ReferenceToken2)"));
	}

	for (auto name : classification2)
	{
		auto node = utils.findNodeByMetadata("Name", name);
		auto metadata = node.getMetadata();
		EXPECT_THAT(metadata["ExternalClassificationReference::Description"], Vs("Reference 3"));
		EXPECT_THAT(metadata["ExternalClassificationReference::Identification"], Vs("myUUID"));
		EXPECT_THAT(metadata["ExternalClassificationReference::Location"], Vs("3drepo.io/classification"));
		EXPECT_THAT(metadata["ExternalClassificationReference::Name"], Vs("ExternalClassificationReference"));
		EXPECT_THAT(metadata["ExternalClassificationReference::Sort"], Vs("TestSortIdentifier"));
	}
}

TEST(IFCModelImport, RelDeclares)
{
	/*
	* IfcRelDeclares is used to associate a set of IfcObjects and/or
	* IfcPropertyDefinitions to an IfcContext (IfcProject) so that they take that
	* context's units and presentation.
	*
	* This is ignored as we only support one context and set of units.
	*/

	SUCCEED();
}

TEST(IFCModelImport, RelDefinesByObject)
{
	/*
	* IfcRelDefinesByObject allows using another object as a prototype for multiple
	* instances. Reflected objects take the Declaring Object's Property Sets and
	* Representations.
	*
	* Unlike DefinesByType, the Reflected object exists as a distinct tangible (if
	* represented) entity in its own right.
	*/

	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse_definesByObject));
	SceneUtils utils(scene);

	auto node = utils.findNodeByMetadata("Name", "Planting_RPC_Tree_Deciduous:Scarlet_Oak-12.5_Meters:328305");
	auto metadata = node.getMetadata();

	EXPECT_THAT(metadata["Pset_BuildingElementProxyCommon::Reference"], Vs("Scarlet_Oak-12.5_Meters"));
	EXPECT_THAT(metadata["Pset_EnvironmentalImpactIndicators::Reference"], Vs("Scarlet_Oak-12.5_Meters"));

	EXPECT_THAT(metadata.find("PSet_1::Length (mm)"), Eq(metadata.end()));

	EXPECT_THAT(metadata.find("PSet_2::Energy (J)"), Eq(metadata.end()));


	node = utils.findNodeByMetadata("Name", "P0");
	metadata = node.getMetadata();

	EXPECT_THAT(metadata["Pset_BuildingElementProxyCommon::Reference"], Vs("Scarlet_Oak-12.5_Meters"));
	EXPECT_THAT(metadata["Pset_EnvironmentalImpactIndicators::Reference"], Vs("Scarlet_Oak-12.5_Meters"));

	EXPECT_THAT(metadata["PSet_1::Length (mm)"], Vq((double)18.3));
	EXPECT_THAT(metadata["PSet_1::Power (W)"], Vq((double)-54));

	EXPECT_THAT(metadata["PSet_2::Energy (J)"], Vq((double)-51));
	EXPECT_THAT(metadata["PSet_2::Force (N)"], Vq((double)238.3));
}

TEST(IFCModelImport, RelDefinesByType)
{
	/*
	* IfcRelDefinesByType is used to declare that an object should inhert the
	* properties of an IfcTypeObject, like a superclass.
	*
	* If a Property with the same name exists in the occuring object, then the
	* occuring object's value takes precedence.
	*/

	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse_definesByType));
	SceneUtils utils(scene);
	auto metadata = utils.findNodeByMetadata("Name", "P0").getMetadata();

	EXPECT_THAT(metadata["PSet_1::Length (mm)"], Vq((double)18.3));
	EXPECT_THAT(metadata["PSet_1::Power (W)"], Vq((double)21.3));

	EXPECT_THAT(metadata["PSet_2::Energy (J)"], Vq((double)38));
	EXPECT_THAT(metadata["PSet_2::Force (N)"], Vq((double)238.3));

	EXPECT_THAT(metadata["PSet_3::Current (A)"], Vq((double)28.3));
	EXPECT_THAT(metadata["PSet_3::Resistance (Ω)"], Vq((double)-11));
}

TEST(IFCModelImport, RelDefinesByTemplate)
{
	/*
	* IfcRelDefinesByTemplate allows declaring that a Property Set is of a given
	* IfcPropertySetTemplate. IfcPropertySetTemplates contain IfcPropertyTemplates.
	*
	* Templates can hold information such as Names and Descriptions, as well as
	* lists of Properties the instantations of the Template should hold.
	*
	* The arrangement does not affect the tree.
	*
	* Currently this way of declaring Properties is redundant, as we have not seen
	* a property that does not hold the relevant information locally, even if
	* templated (see for example, the attributes of the Predefined Property Sets).
	*/

	SUCCEED();
}

TEST(IFCModelImport, ValuesAndUnits)
{
	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse_valuesAndUnits));
	SceneUtils utils(scene);

	auto node = utils.findNodeByMetadata("Name", "P0");
	auto metadata = node.getMetadata();

	/*
	* This test checks that all named IfcMeasureResource types are correctly
	* handled. This test also checks the behaviour of the units, since the measure
	* types and units processing are interwoven.
	*
	* The units checked here are set at the Project (Context) level in the file,
	* and cover SI, Derived and Context Dependent units.
	*
	* Conversion (Imperial) units and MeasureWithUnit are covered by other tests.
	*/

	EXPECT_THAT(metadata["PSet_MeasureValues::Volume (m³)"], Vq(48.4));
	EXPECT_THAT(metadata["PSet_MeasureValues::Time1 (s)"], Vq(1.8));
	EXPECT_THAT(metadata["PSet_MeasureValues::Time2 (s)"], Vq(-1.8));
	EXPECT_THAT(metadata["PSet_MeasureValues::Temp1 (K)"], Vq(68.9));
	EXPECT_THAT(metadata["PSet_MeasureValues::Temp2 (K)"], Vq(-68.9));
	EXPECT_THAT(metadata["PSet_MeasureValues::SolidAngle (sr)"], Vq(1.3));
	EXPECT_THAT(metadata["PSet_MeasureValues::Ratio1"], Vq(0.25));
	EXPECT_THAT(metadata["PSet_MeasureValues::Ratio2"], Vq(-0.25));
	EXPECT_THAT(metadata["PSet_MeasureValues::Angle1 (rad)"], Vq(2.1));
	EXPECT_THAT(metadata["PSet_MeasureValues::Angle2 (rad)"], Vq(-2.1));
	EXPECT_THAT(metadata["PSet_MeasureValues::Amount1"], Vq(219.358));
	EXPECT_THAT(metadata["PSet_MeasureValues::Amount2"], Vq(-219.358));
	EXPECT_THAT(metadata["PSet_MeasureValues::Count1"], Vq((double)152)); // Counts are NUMBERs, which are encoded as double
	EXPECT_THAT(metadata["PSet_MeasureValues::Mass (kg)"], Vq(11.7));
	EXPECT_THAT(metadata["PSet_MeasureValues::Length1 (mm)"], Vq(48.4));
	EXPECT_THAT(metadata["PSet_MeasureValues::Length2 (mm)"], Vq(-48.4));
	EXPECT_THAT(metadata["PSet_MeasureValues::Length3 (mm)"], Vq((double)0));
	EXPECT_THAT(metadata["PSet_MeasureValues::Length4 (mm)"], Vq(0.01));
	EXPECT_THAT(metadata["PSet_MeasureValues::Current (A)"], Vq(30.1));
	EXPECT_THAT(metadata["PSet_MeasureValues::Desc"], Vs("A Description"));
	EXPECT_THAT(metadata["PSet_MeasureValues::Count2"], Vq(18));
	EXPECT_THAT(metadata["PSet_MeasureValues::Context"], Vq(99.46));
	EXPECT_THAT(metadata["PSet_MeasureValues::Area (m²)"], Vq(101.4));
	EXPECT_THAT(metadata["PSet_MeasureValues::Amount (mol)"], Vq(849.14));
	EXPECT_THAT(metadata["PSet_MeasureValues::Luminosity (cd)"], Vq(1800.9));
	EXPECT_THAT(metadata["PSet_MeasureValues::Norm"], Vq(0.9));
	EXPECT_THAT(metadata["PSet_MeasureValues::Imaginary"], Vs("78.400000+123.100000i"));

	EXPECT_THAT(metadata["PSet_SimpleValues::Integer1"], Vq(-123));
	EXPECT_THAT(metadata["PSet_SimpleValues::Integer2"], Vq(0));
	EXPECT_THAT(metadata["PSet_SimpleValues::Integer3"], Vq(123));
	EXPECT_THAT(metadata["PSet_SimpleValues::Integer4"], Vq(567));
	EXPECT_THAT(metadata["PSet_SimpleValues::Real"], Vq(-904.341));
	EXPECT_THAT(metadata["PSet_SimpleValues::Boolean"], Vq(true));
	EXPECT_THAT(metadata["PSet_SimpleValues::Identifer"], Vs("MyIdentifier"));
	EXPECT_THAT(metadata["PSet_SimpleValues::Text"], Vs("MyText"));
	EXPECT_THAT(metadata["PSet_SimpleValues::Label"], Vs("MyLabel"));

	// Logicals do not work due to a known bug in IFCOS
	// EXPECT_THAT(metadata["PSet_SimpleValues::Logical"], Vs("Unknown"));

	EXPECT_THAT(metadata["PSet_SimpleValues::DateTime"], Vs("2025-07-03 10:24"));
	EXPECT_THAT(metadata["PSet_SimpleValues::Date"], Vs("2025-07-03"));
	EXPECT_THAT(metadata["PSet_SimpleValues::Time"], Vs("10:24"));
	EXPECT_THAT(metadata["PSet_SimpleValues::Duration"], Vs("3 Hours"));
	time_t time = 1741343169;
	std::tm tm = *gmtime(&time);
	EXPECT_THAT(metadata["PSet_SimpleValues::Timestamp"], Vq(tm));

	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::VolumetricFlowRate (m³·s⁻¹)"], Vq(101.3));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ThermalTransmittance (kg·K⁻¹·s⁻³)"], Vq(10.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ThermalResistance (kg⁻¹·m⁻²·s³·K)"], Vq(1.98));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ThermalAdmittance (W·m⁻²·K⁻¹)"], Vq(98.7));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Pressure (Pa)"], Vq(187.5));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Power (W)"], Vq(588.9));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MassFlowRate (kg·m³·s⁻¹)"], Vq(879.54));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MassDensity (kg·m⁻³)"], Vq(87.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LinearVelocity (mm·s⁻¹)"], Vq(10.8));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::KinematicViscosity (m²·s⁻¹)"], Vq(1.99));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::IntegerCount"], Vq(987));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::HeatFluxDensity (W⁻¹·m⁻²)"], Vq(778.7));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Frequency (Hz)"], Vq(44.89));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Energy (J)"], Vq(509.63));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Voltage (V)"], Vq(5.5));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::DynamicViscosity (N·s·m⁻²)"], Vq(778.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::CompoundPlaneAngle"], Vs("-58° 32' 57\" 121678"));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::AngularVelocity (rad·s⁻¹)"], Vq(69.2));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ThermalConductivity (kg·m²·s⁻³·K⁻¹)"], Vq(78.14));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MolecularWeight (kg·mol⁻¹)"], Vq(1008.7));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::VaporPermeability (kg·m⁻²·s⁻¹)"], Vq(12.19));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MoistureDiffusivity (m²·s⁻¹)"], Vq(778.12));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::IsothermalMoistureCapacity (kg·m⁻³)"], Vq(423.8));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SpecificHeatCapacity (J·kg⁻¹·K⁻¹)"], Vq(7897.11));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MonetaryMeasure (GBP)"], Vq(10896.34));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MagneticFluxDensity (T)"], Vq(889.5));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MagneticFlux (Wb)"], Vq(877.2));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LuminousFlux (lm)"], Vq(354.2));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Force (N)"], Vq(24.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Inductance (H)"], Vq(0.0384));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Illuminance (lx)"], Vq(558.9));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ElectricResistance (Ω)"], Vq(10001.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ElectricConductance (S)"], Vq(0.1878));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ElectricCharge (C)"], Vq(34.2));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::DoseEquivalent (Sv)"], Vq(11.84));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ElectricCapacitance (F)"], Vq(0.587));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::AbsorbedDose (Gy)"], Vq(0.488));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::RadioActivity (Bq)"], Vq(1018.5));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::RotationalFrequency (rad·s⁻¹)"], Vq(1000.8));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Torque (N·mm)"], Vq(110.2));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Acceleration (mm·s⁻²)"], Vq(55.23));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LinearForce (N·mm⁻¹)"], Vq(188.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LinearStiffness (N·mm⁻¹)"], Vq(7894.14));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ModulusOfSubgradeReaction (N·m⁻²·mm⁻¹)"], Vq(65.31));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ModulusOfElasticity (N·m⁻²)"], Vq(90.34));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MomentOfInertia (kg·m²)"], Vq(33.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::PlanarForce (N·m⁻²)"], Vq(22.5));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::RotationalStiffness (N·rad⁻¹)"], Vq(76.26));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ShearModulus (N·m⁻²)"], Vq(42.65));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LinearMoment (kg·mm·s⁻¹)"], Vq(30.03));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LuminousIntensityDistribution (lm·sr⁻¹)"], Vq(93.57));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Curvature (mm⁻¹)"], Vq(107.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MassPerLength (kg·mm⁻¹)"], Vq(23.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ModulusOfLinearSubgradeReaction (kg·m⁻²·mm⁻¹)"], Vq(111.105));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ModulusOfRotationalSubgradeReaction (kg·m⁻²·rad⁻¹)"], Vq(50.7));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::RotationalMass (rad·kg)"], Vq(101.8));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SectionalAreaIntegral (m²)"], Vq(59.55));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SectionModulus (m²)"], Vq(28.3));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::TemperatureGradient (K·mm⁻¹)"], Vq(56.004));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ThermalExpansionCoefficient (mm⁻¹·K⁻¹)"], Vq(68.531));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::WarpingConstant (N·m²·mm·kg)"], Vq(67.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::WarpingMoment (mm⁻¹·rad·N)"], Vq(2.7866));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SoundPower (W)"], Vq(58.7));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SoundPressure (Pa)"], Vq(45.2));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::HeatingValue (J·kg⁻¹)"], Vq(27.30));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::PH"], Vq(7.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::IfcIonConcentration (mol·m⁻³)"], Vq(28.3));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::TemperatureRateOfChange (K·s⁻¹)"], Vq(99.52));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::AreaDensity (kg·m⁻²)"], Vq(23.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SoundPowerLevel (db)"], Vq(9.08));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SoundPressureLevel (db)"], Vq(5.8));
}

TEST(IFCModelImport, RootAttributes)
{
	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse1));
	SceneUtils utils(scene);

	auto metadata = utils.findNodeByMetadata("Name", "Default").getMetadata();

	/*
	* IfcRoot attributes should be imported with different names than their IFC
	* Attribute Names.
	*
	* Attributes should also support the full units functionality based on their
	* named types (if any).
	*/

	EXPECT_THAT(metadata["CompositionType"], Vs("ELEMENT"));
	EXPECT_THAT(metadata["IFC GUID"], Vs("1NUtDGAAj0sAcNoGljf6v$"));
	EXPECT_THAT(metadata["IFC Type"], Vs("IfcSite"));
	EXPECT_THAT(metadata["Name"], Vs("Default"));
	EXPECT_THAT(metadata["RefElevation (mm)"], Vq((double)0));
	EXPECT_THAT(metadata["RefLatitude"], Vs("51° 30' 23\" 112487"));
	EXPECT_THAT(metadata["RefLongitude"], Vs("0° 7' 37\" 956022"));
}

TEST(IFCModelImport, PropertySets)
{
	/*
	* Ifc Property Sets can contain a mix of Measure Values and other values, such
	* as enumerations and lists. This test ensures that all Property sub-types are
	* parsed and formatted correctly, including units.
	*
	* Complex Properties should be unrolled, with the set name or usage suffixed to
	* each sub-property.
	*/

	auto scene = IfcModelImportUtils::ModelImportManagerImport("PropertySets", getDataPath(ifcSimpleHouse_propertySets));
	SceneUtils utils(scene);

	auto metadata = utils.findNodeByMetadata("Name", "P0").getMetadata();

	EXPECT_THAT(metadata["PSet_Mixed::AppliedIfcMeasureReference (mm)"], Vq((double)22));
	EXPECT_THAT(metadata["PSet_Mixed::AppliedIfcMeasureWithUnitReference (W)"], Vq((double)1));
	EXPECT_THAT(metadata["PSet_Mixed::BoundedValueDefaultUnits (mm)"], Vs("[99.100000, 0.100000]"));
	EXPECT_THAT(metadata["PSet_Mixed::BoundedValueExplicitUnits (m)"], Vs("[12, 0]"));
	EXPECT_THAT(metadata["PSet_Mixed::BoundedValueNoUnits"], Vs("[9, 1]"));
	EXPECT_THAT(metadata["PSet_Mixed::EnumLabel"], Vs("MyLabel"));
	EXPECT_THAT(metadata["PSet_Mixed::EnumLabelWithReference"], Vs("(Label1, Label2)"));
	EXPECT_THAT(metadata["PSet_Mixed::EnumMeasure (mm)"], Vq((double)1));
	EXPECT_THAT(metadata["PSet_Mixed::EnumMeasureWithUnit (m)"], Vq((double)1000));
	EXPECT_THAT(metadata["PSet_Mixed::ListMeasureValues (N)"], Vq((double)112));
	EXPECT_THAT(metadata["PSet_Mixed::ListSimpleValues"], Vs("MyLabel")); // Lists with only one value are not bracketed
	EXPECT_THAT(metadata["PSet_Mixed::ListValueWithUnit (W)"], Vq((double)112));
	EXPECT_THAT(metadata["PSet_Mixed::ListValueWithImperialUnit (lbf)"], Vq((double)112));
	EXPECT_THAT(metadata["PSet_Mixed::SingleInteger1"], Vq(-123));
	EXPECT_THAT(metadata["PSet_Mixed::SingleVolume (m³)"], Vq((double)48.4));
	EXPECT_THAT(metadata["PSet_Mixed::SingleVolumetricFlowRate (m³·s⁻¹)"], Vq((double)101.3));

	metadata = utils.findNodeByMetadata("Name", "P1").getMetadata();

	EXPECT_THAT(metadata["PSet_A::Length (mm)"], Vq((double)18.3));
	EXPECT_THAT(metadata["PSet_A::Power (W)"], Vq((double)-54));

	EXPECT_THAT(metadata["PSet_B::Energy (J)"], Vq((double)-51));
	EXPECT_THAT(metadata["PSet_B::Force (N)"], Vq((double)238.3));

	EXPECT_THAT(metadata["PSet_C::Current (A)"], Vq((double)28.3));
	EXPECT_THAT(metadata["PSet_C::Resistance (Ω)"], Vq((double)-11));

	EXPECT_THAT(metadata["PSet_Complex::SubForce (N)"], Vq((double)72.9));
	EXPECT_THAT(metadata["PSet_Complex::SubForce (Inner) (N)"], Vq((double)723.59));
	EXPECT_THAT(metadata["PSet_Complex::SubForce (Outer) (N)"], Vq((double)65.6));
	EXPECT_THAT(metadata["PSet_Complex::SubInteger1"], Vq(-54));
	EXPECT_THAT(metadata["PSet_Complex::SubInteger1 (Inner)"], Vq(-344));
	EXPECT_THAT(metadata["PSet_Complex::SubInteger1 (Outer)"], Vq(3));
	EXPECT_THAT(metadata["PSet_Complex::SubVolume (m³)"], Vq((double)18.3));
	EXPECT_THAT(metadata["PSet_Complex::SubVolume (Inner) (m³)"], Vq((double)28.3));
	EXPECT_THAT(metadata["PSet_Complex::SubVolume (Outer) (m³)"], Vq((double)134.3));
}

TEST(IFCModelImport, PropertySetsOfSets)
{
	/*
	* PropertySetDefinitionSets are not supported (https://github.com/IfcOpenShell/IfcOpenShell/issues/6330)
	* The importer will not populate them, but should not crash.
	*/

	auto scene = IfcModelImportUtils::ModelImportManagerImport("PropertySetSets", getDataPath(ifcSimpleHouse_propertySetsOfSets));
	SceneUtils utils(scene);

	auto metadata = utils.findNodeByMetadata("Name", "P1").getMetadata();

	// If the above succeeds, the node has been successfully imported.

	SUCCEED();
}

TEST(IFCModelImport, PredefinedPropertySets)
{
	auto scene = IfcModelImportUtils::ModelImportManagerImport("PredefinedPropertySets", getDataPath(ifcSimpleHouse_predefinedPropertySets));
	SceneUtils utils(scene);

	auto metadata = utils.findNodeByMetadata("Name", "P0").getMetadata();

	/*
	* Predefined property sets are property sets that are statically defined in the
	* Ifc spec.
	*
	* Something specifically special to PredefinedProperySets is that their
	* elements can contain object references. While IfcSimpleProperty can hold
	* an IfcObjectReferenceSelect, this is limited in the number of types the
	* reference can be. Predefined Property Sets on the other hand reference a
	* graph of IfcPreDefinedProperties.
	*
	* This test checks both simple predefinde properties and those with recursive
	* reference attributes.
	*/

	EXPECT_THAT(metadata["IfcDoorPanelProperties::PanelDepth (mm)"], Vq((double)50));
	EXPECT_THAT(metadata["IfcDoorPanelProperties::PanelOperation"], Vs("SWINGING"));
	EXPECT_THAT(metadata["IfcDoorPanelProperties::PanelPosition"], Vs("MIDDLE"));
	EXPECT_THAT(metadata["IfcDoorPanelProperties::PanelWidth"], Vq(0.81));

	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::BarCount (ReinforcementSectionDefinitions 0:CrossSectionReinforcementDefinitions 0)"], Vq(1));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::BarCount (ReinforcementSectionDefinitions 1:CrossSectionReinforcementDefinitions 0)"], Vq(1));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::BarSurface (ReinforcementSectionDefinitions 0:CrossSectionReinforcementDefinitions 0)"], Vs("PLAIN"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::BarSurface (ReinforcementSectionDefinitions 1:CrossSectionReinforcementDefinitions 0)"], Vs("PLAIN"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::DefinitionType"], Vs("Reinforcement"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::EffectiveDepth (ReinforcementSectionDefinitions 0:CrossSectionReinforcementDefinitions 0) (mm)"], Vq((double)50));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::EffectiveDepth (ReinforcementSectionDefinitions 1:CrossSectionReinforcementDefinitions 0) (mm)"], Vq((double)100));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::LongitudinalEndPosition (ReinforcementSectionDefinitions 0) (mm)"], Vq((double)1));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::LongitudinalEndPosition (ReinforcementSectionDefinitions 1) (mm)"], Vq((double)2));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::LongitudinalStartPosition (ReinforcementSectionDefinitions 0) (mm)"], Vq((double)0));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::LongitudinalStartPosition (ReinforcementSectionDefinitions 1) (mm)"], Vq((double)1));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::NominalBarDiameter (ReinforcementSectionDefinitions 0:CrossSectionReinforcementDefinitions 0) (mm)"], Vq((double)10));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::NominalBarDiameter (ReinforcementSectionDefinitions 1:CrossSectionReinforcementDefinitions 0) (mm)"], Vq((double)9));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ProfileName (ReinforcementSectionDefinitions 0:SectionDefinition:EndProfile)"], Vs("Area Section Profile End"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ProfileName (ReinforcementSectionDefinitions 0:SectionDefinition:StartProfile)"], Vs("Area Section Profile Start"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ProfileName (ReinforcementSectionDefinitions 1:SectionDefinition:EndProfile)"], Vs("Area Section Profile End"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ProfileName (ReinforcementSectionDefinitions 1:SectionDefinition:StartProfile)"], Vs("Area Section Profile Start"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ProfileType (ReinforcementSectionDefinitions 0:SectionDefinition:EndProfile)"], Vs("AREA"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ProfileType (ReinforcementSectionDefinitions 0:SectionDefinition:StartProfile)"], Vs("AREA"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ProfileType (ReinforcementSectionDefinitions 1:SectionDefinition:EndProfile)"], Vs("AREA"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ProfileType (ReinforcementSectionDefinitions 1:SectionDefinition:StartProfile)"], Vs("AREA"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ReinforcementRole (ReinforcementSectionDefinitions 0)"], Vs("STUD"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::ReinforcementRole (ReinforcementSectionDefinitions 1)"], Vs("STUD"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::SectionType (ReinforcementSectionDefinitions 0:SectionDefinition)"], Vs("TAPERED"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::SectionType (ReinforcementSectionDefinitions 1:SectionDefinition)"], Vs("TAPERED"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::SteelGrade (ReinforcementSectionDefinitions 0:CrossSectionReinforcementDefinitions 0)"], Vs("A1"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::SteelGrade (ReinforcementSectionDefinitions 0:CrossSectionReinforcementDefinitions 0)"], Vs("A1"));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::TotalCrossSectionArea (ReinforcementSectionDefinitions 0:CrossSectionReinforcementDefinitions 0) (m²)"], Vq(2.3));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::TotalCrossSectionArea (ReinforcementSectionDefinitions 1:CrossSectionReinforcementDefinitions 0) (m²)"], Vq(5.3));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::TransversePosition (ReinforcementSectionDefinitions 0) (mm)"], Vq(0.5));
	EXPECT_THAT(metadata["IfcReinforcementDefinitionProperties::TransversePosition (ReinforcementSectionDefinitions 1) (mm)"], Vq(0.75));
}

TEST(IFCModelImport, QuantitySets)
{
	auto scene = IfcModelImportUtils::ModelImportManagerImport("QuantitySets", getDataPath(ifcSimpleHouse_quantitySets));
	SceneUtils utils(scene);
	auto metadata = utils.findNodeByMetadata("Name", "P0").getMetadata();

	/*
	* Quantity Sets are a type sibling of IfcPropertySet, behaving very similarly
	* but limited to only IfcPhysicalQuantity entries.
	*
	* The type also has an optional MethodOfMeasurement attribute, which we add
	* in parallel with the members.
	*/

	EXPECT_THAT(metadata["Quantities_DefaultUnits::Area (m²)"], Vq((double)25));
	EXPECT_THAT(metadata["Quantities_DefaultUnits::Count"], Vq(12));
	EXPECT_THAT(metadata["Quantities_DefaultUnits::Length (mm)"], Vq((double)9.9));
	EXPECT_THAT(metadata["Quantities_DefaultUnits::Time (s)"], Vq((double)1.8));
	EXPECT_THAT(metadata["Quantities_DefaultUnits::Volume (m³)"], Vq((double)10.4));
	EXPECT_THAT(metadata["Quantities_DefaultUnits::Weight (kg)"], Vq((double)109.8));

	EXPECT_THAT(metadata.find("Quantities_DefaultUnits::MethodOfMeasurement"), Eq(metadata.end()));

	EXPECT_THAT(metadata["Quantities_ExplicitUnits::Area (V)"], Vq((double)25));
	EXPECT_THAT(metadata["Quantities_ExplicitUnits::Count (V)"], Vq(12));
	EXPECT_THAT(metadata["Quantities_ExplicitUnits::Length (V)"], Vq((double)9.9));
	EXPECT_THAT(metadata["Quantities_ExplicitUnits::Time (V)"], Vq((double)1.8));
	EXPECT_THAT(metadata["Quantities_ExplicitUnits::Volume (V)"], Vq((double)10.4));
	EXPECT_THAT(metadata["Quantities_ExplicitUnits::Weight (V)"], Vq((double)109.8));

	EXPECT_THAT(metadata["Quantities_ExplicitUnits::MethodOfMeasurement"], Vs("With Explicit Units"));

	EXPECT_THAT(metadata["Quantities_WithComplex::Area (Complex Quantity) (s)"], Vq((double)16));
	EXPECT_THAT(metadata["Quantities_WithComplex::Area (V)"], Vq((double)25));
	EXPECT_THAT(metadata["Quantities_WithComplex::Count (Complex Quantity) (W)"], Vq(5));
	EXPECT_THAT(metadata["Quantities_WithComplex::Count (V)"], Vq(12));
	EXPECT_THAT(metadata["Quantities_WithComplex::Length (Complex Quantity) (mm)"], Vq((double)102));
	EXPECT_THAT(metadata["Quantities_WithComplex::Length (mm)"], Vq((double)9.9));
	EXPECT_THAT(metadata["Quantities_WithComplex::Time (Complex Quantity) (s)"], Vq((double)7.8));
	EXPECT_THAT(metadata["Quantities_WithComplex::Time (s)"], Vq(1.8));
	EXPECT_THAT(metadata["Quantities_WithComplex::Volume (Complex Quantity) (m³)"], Vq((double)8.4));
	EXPECT_THAT(metadata["Quantities_WithComplex::Volume (s)"], Vq((double)10.4));
	EXPECT_THAT(metadata["Quantities_WithComplex::Weight (Complex Quantity) (kg)"], Vq((double)10.87));
	EXPECT_THAT(metadata["Quantities_WithComplex::Weight (kg)"], Vq((double)109.8));

	EXPECT_THAT(metadata["Quantities_WithComplex::MethodOfMeasurement"], Vs("With Complex Quantities"));
}

TEST(IFCModelImport, ImperialUnits)
{
	auto scene = IfcModelImportUtils::ModelImportManagerImport("QuantitySets", getDataPath(ifcSimpleHouse_imperialUnits));
	SceneUtils utils(scene);
	auto metadata = utils.findNodeByMetadata("Name", "P0").getMetadata();

	/*
	* IFCOS has the concept of a number of imperial units which can be mapped to
	* named types and derived units.
	*/

	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Acceleration (in·day⁻²)"], Vq((double)55.23));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::AreaDensity (t(UK)·in⁻²)"], Vq((double)23.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Curvature (in⁻¹)"], Vq((double)107.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::DynamicViscosity (lbf·day·in⁻²)"], Vq((double)778.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Energy (btu)"], Vq((double)509.63));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Force (lbf)"], Vq((double)24.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::HeatingValue (btu·t(UK)⁻¹)"], Vq((double)27.3));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::IsothermalMoistureCapacity (t(UK)·ft⁻³)"], Vq((double)423.8));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::KinematicViscosity (in²·day⁻¹)"], Vq((double)1.99));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LinearForce (lbf·in⁻¹)"], Vq((double)188.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LinearMoment (t(UK)·in·day⁻¹)"], Vq((double)30.03));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LinearStiffness (lbf·in⁻¹)"], Vq((double)7894.14));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::LinearVelocity (in·day⁻¹)"], Vq((double)10.8));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MassDensity (t(UK)·ft⁻³)"], Vq((double)87.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MassFlowRate (t(UK)·ft³·day⁻¹)"], Vq((double)879.54));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MassPerLength (t(UK)·in⁻¹)"], Vq((double)23.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ModulusOfElasticity (lbf·in⁻²)"], Vq((double)90.34));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ModulusOfLinearSubgradeReaction (t(UK)·in⁻²·in⁻¹)"], Vq((double)111.105));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ModulusOfSubgradeReaction (lbf·in⁻²·in⁻¹)"], Vq((double)65.31));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MoistureDiffusivity (in²·day⁻¹)"], Vq((double)778.12));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::MomentOfInertia (t(UK)·in²)"], Vq((double)33.1));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::PlanarForce (lbf·in⁻²)"], Vq((double)22.5));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Pressure (psi)"], Vq((double)187.5));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SectionModulus (in²)"], Vq((double)28.3));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SectionalAreaIntegral (in²)"], Vq((double)59.55));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ShearModulus (lbf·in⁻²)"], Vq((double)42.65));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SoundPowerLevel (db)"], Vq((double)9.08));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SoundPressure (psi)"], Vq((double)45.2));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::SoundPressureLevel (db)"], Vq((double)5.8));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::TemperatureGradient (°F·in⁻¹)"], Vq((double)56.004));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::TemperatureRateOfChange (°F·day⁻¹)"], Vq((double)99.52));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ThermalConductivity (t(UK)·in²·day⁻³·°F⁻¹)"], Vq((double)78.14));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ThermalExpansionCoefficient (in⁻¹·°F⁻¹)"], Vq((double)68.531));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ThermalResistance (t(UK)⁻¹·in⁻²·day³·°F)"], Vq((double)1.98));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::ThermalTransmittance (kg·K⁻¹·s⁻³)"], Vq((double)10.4));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::Torque (lbf·in)"], Vq((double)110.2));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::VaporPermeability (t(UK)·in⁻²·day⁻¹)"], Vq((double)12.19));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::VolumetricFlowRate (ft³·day⁻¹)"], Vq((double)101.3));
	EXPECT_THAT(metadata["PSet_DerivedMeasureValues::WarpingConstant (lbf·in²·in·t(UK))"], Vq((double)67.4));

	EXPECT_THAT(metadata["PSet_MeasureValues::Angle1 (°)"], Vq((double)2.1));
	EXPECT_THAT(metadata["PSet_MeasureValues::Area (in²)"], Vq((double)101.4));
	EXPECT_THAT(metadata["PSet_MeasureValues::Length1 (in)"], Vq((double)48.4));
	EXPECT_THAT(metadata["PSet_MeasureValues::Mass (t(UK))"], Vq((double)11.7));
	EXPECT_THAT(metadata["PSet_MeasureValues::Temp1 (°F)"], Vq((double)68.9));
	EXPECT_THAT(metadata["PSet_MeasureValues::Time1 (day)"], Vq((double)1.8));
	EXPECT_THAT(metadata["PSet_MeasureValues::Volume (ft³)"], Vq((double)48.4));
}

TEST(IFCModelImport, Unicode)
{
	/*
	* The metadata tests above do consider special characters, however the tests
	* & importer are built by the same toolchain, so they won't detect if a
	* lossless, but undesirable, codepage (such as Windows-1252) is used.
	*
	* This test makes sure that the text encodings are UTF8 at the byte level.
	*/

	auto scene = IfcModelImportUtils::ModelImportManagerImport("Duct", getDataPath(ifcModel2));
	SceneUtils utils(scene);

	auto metadata = utils.findNodeByMetadata("Name", "Oval Duct:Standard:329435").getMetadata();
	for (auto& m : metadata)
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

TEST(IFCModelImport, Materials)
{
	auto scene = IfcModelImportUtils::ModelImportManagerImport("SimpleHouse", getDataPath(ifcSimpleHouse1));
	SceneUtils utils(scene);

	// The Toposolid does not have a material definition, so we should be assinging
	// a sensible default

	auto defaultIfcMaterial = repo::lib::repo_material_t::DefaultMaterial();
	defaultIfcMaterial.shininess = 0.5;
	defaultIfcMaterial.shininessStrength = 0.5;

	for (auto& m : utils.findNodeByMetadata("Name", "Toposolid:Toposolid 1:328663").getMeshes())
	{
		EXPECT_THAT(m.getMaterial(), Eq(defaultIfcMaterial));
	}

	auto brickMaterial = repo::lib::repo_material_t::DefaultMaterial();
	brickMaterial.diffuse = repo::lib::repo_color3d_t(0.666666687, 0.392156869, 0.411764711);
	brickMaterial.ambient = repo::lib::repo_color3d_t();
	brickMaterial.specular = repo::lib::repo_color3d_t(0.5, 0.5, 0.5);
	brickMaterial.emissive = repo::lib::repo_color3d_t(0.5, 0.5, 0.5);
	brickMaterial.opacity = 1;
	brickMaterial.shininess = 128;
	brickMaterial.shininessStrength = 0.5;

	for (auto& m : utils.findNodeByMetadata("Name", "Basic Wall:Wall-Ext_102Bwk-75Ins-100LBlk-12P:326450").getMeshes())
	{
		EXPECT_THAT(m.getMaterial(), Eq(brickMaterial));
	}
}

TEST(IFCModelImport, InfiniteLoopRegression)
{
	/*
	* This test makes sure the infinite regression bug described here is still
	* resolved: https://tracker.dev.opencascade.org/view.php?id=32949
	* We have a fix for OCCT if it regresses here: https://github.com/3drepo/OCCT/tree/CR0032949
	*/

	auto scene = IfcModelImportUtils::ModelImportManagerImport("RegressionTests", getDataPath(ifcModel_InfiniteLoop));
	SceneUtils utils(scene);

	if (utils.findNodeByMetadata("Name", "N4021").node)
	{
		SUCCEED();
	}
	else
	{
		FAIL();
	}
}