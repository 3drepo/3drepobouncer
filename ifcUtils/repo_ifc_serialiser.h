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

/*
* This header is a template that along with the repo_ifc_serialiser.cpp module
* defines a serialiser for a given schema.
* A few things to consider when working on a serialiser:
*	1.	This file does not have a header guard, because it is expected to be
*		included in ifcUtils multiple times with different defines.
*	2.	The repo_ifc_serialiser.cpp module is compiled multiple times, so make sure
*		any helper functions are static to avoid multiple defines.
*	3.	Ensure that the file is saved as UTF-8 so that the compiler will use the
*		correct encodings for literals in the named formats. The repo_ifc_serialiser.cpp
*		file has been saved with the BOM to add an extra check for if this changes.
*	4.	Do not use the SCHEMA_HAS... defines inside the header, as these don't go
*		away when the IfcSchema definition changes; put any such conditions as static
*		functions in the repo_ifc_serialiser.cpp file.
*/

#include "repo_ifc_serialiser_abstract.h"

#include "repo/manipulator/modelutility/repo_scene_builder.h"
#include "repo/lib/datastructure/repo_bounds.h"

#include <memory>

#include <ifcparse/IfcFile.h>
#include <ifcgeom/Iterator.h>
#include <ifcgeom/IfcGeomElement.h>
#include <ifcgeom/ConversionSettings.h>

// The indirection is required to get the preprocessor evaluate the IfcSchema
// definition

#define CLASS_NAME(name) CLASS_NAME_IMPL(name)
#define CLASS_NAME_IMPL(name) name##Serialiser

#define HEADER_NAME(name) HEADER_NAME_IMPL(name)
#define HEADER_NAME_IMPL(name) <ifcparse/##name.h>

#define DEFINITIONS_NAME(name) DEFINITIONS_NAME_IMPL(name)
#define DEFINITIONS_NAME_IMPL(name) <ifcparse/##name-definitions.h>

#include HEADER_NAME(IfcSchema)
#include DEFINITIONS_NAME(IfcSchema)

#define IfcSerialiser CLASS_NAME(IfcSchema)

namespace ifcUtils 
{
	/*
	* The IfcSerialiser behaves similarly to a GeometrySerialiser of
	* IfcOpenShell. It processes a filtered list of elements, and builds the
	* tree around them (as oppposed to performing a forward traversal of the
	* tree).
	*/
	class IfcSerialiser : public AbstractIfcSerialiser
	{
	public:
		IfcSerialiser(std::unique_ptr<IfcParse::IfcFile> file);

		virtual void import(repo::manipulator::modelutility::RepoSceneBuilder* builder) override;

	protected:
		std::unique_ptr<IfcParse::IfcFile> file;
		ifcopenshell::geometry::Settings settings;
		std::string kernel;
		int numThreads;
		repo::manipulator::modelutility::RepoSceneBuilder* builder;
		repo::lib::RepoUUID rootNodeId;
		std::unordered_map<std::string, repo::lib::repo_material_t> materials;
		std::unordered_map<int64_t, repo::lib::RepoUUID> sharedIds;

		enum class RepoDerivedUnits {
			MONETARY_VALUE
		};

		using unit_t = std::variant<RepoDerivedUnits, IfcSchema::IfcUnitEnum::Value, IfcSchema::IfcDerivedUnitEnum::Value>;

		std::unordered_map<int, unit_t> definedValueTypeUnits;
		std::unordered_map<unit_t, std::string> unitLabels;

		struct filter;

		/*
		* Gets the bounds from the file and updates the scene builder and importer
		* settings so all geometry is imported with the min world offset.
		*/
		void updateBounds();

		void updateUnits();

		void createRootNode();

		void configureSettings();

		repo::lib::RepoBounds getBounds();

		const repo::lib::repo_material_t& resolveMaterial(
			const ifcopenshell::geometry::taxonomy::style::ptr ptr);

		struct Formatted {
			std::optional<repo::lib::RepoVariant> v;
			std::string units;
		};

		std::string getUnitsLabel(const IfcSchema::IfcUnit* unit);
		std::string getUnitsLabel(const IfcSchema::IfcSIUnitName::Value& name);
		std::string getUnitsLabel(const IfcSchema::IfcSIPrefix::Value& prefix);
		std::string getUnitsLabel(const std::string& name);
		std::string getUnitsLabel(const unit_t& units);
		std::string getUnitsLabel(const IfcParse::declaration& type);

		std::string getExponentAsString(int value);

		/*
		* Sets the unit_t that will be used by the getUnitsLabel declaration overide.
		* This should be called for all named Ifc types that have an implicit unit
		* defined by the IfcMeasureResource - this must be done explicitly because no
		* such mapping exists in IFCOS currently.
		*/

		void setUnits(const IfcParse::type_declaration& type, unit_t units);
		void setUnitsLabel(unit_t unit, const IfcSchema::IfcUnit* definition);

		void setUnits(IfcSchema::IfcSIUnit u);

		/*
		* Creates the associations between Ifc Classes and Unit Enums that are expressed
		* in the IfcMeasureResource, but are not programmatically linked in IFCOS. Should
		* be called early on (e.g. in the constructor).
		*/
		void setDefinedTypeUnits();
		
		/*
		* Creates the MeshNode(s) for a given element - this may consist of a
		* number of primitive sets, with multiple materials. Will also create
		* the tree and metadata nodes.
		*/
		void import(const IfcGeom::TriangulationElement* triangulation);

		/*
		* Given an arbitrary Ifc Object, determine from its relationships which is
		* the best one for it so it under in the acyclic tree.
		*/
		const IfcSchema::IfcObjectDefinition* getParent(const IfcSchema::IfcObjectDefinition* object);

		/*
		* Creates the tree entry for the object, and recursively all its parents
		* upwards towards the project root.
		*
		* How the tree is built depends on the object's own type and the
		* relationships available to it.
		*
		* Objects are things that semantically exist, but are not necessarily
		* present in the physical world - that is, they could include walls, doors,
		* but also storeys and projects. Both types of object coexist in the tree.
		*
		*/
		repo::lib::RepoUUID createTransformationNode(const IfcSchema::IfcObjectDefinition* object);

		/*
		* Creates a transformation node under the parent object to group together
		* entities of a given type.
		*/
		repo::lib::RepoUUID createTransformationNode(
			const IfcSchema::IfcObjectDefinition* parent,
			const IfcParse::entity& type);

		std::unique_ptr<repo::core::model::MetadataNode> createMetadataNode(
			const IfcSchema::IfcObjectDefinition* object);

		/*
		* Builds metadata and possibly transformation nodes for this element. If
		* the caller passes false to createTransform, then it is expected any mesh
		* nodes will attach directly to the parents.
		*/
		repo::lib::RepoUUID getParentId(
			const IfcGeom::Element* element,
			bool createTransform);

		Formatted processAttributeValue(
			const IfcParse::attribute* attribute,
			const AttributeValue& value,
			std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

		void collectAdditionalAttributes(
			const IfcSchema::IfcObjectDefinition* object,
			std::unordered_map<std::string, repo::lib::RepoVariant>& metadata);

		bool shouldGroupByType(
			const IfcSchema::IfcObjectDefinition* object,
			const IfcSchema::IfcObjectDefinition* parent);
	};
}