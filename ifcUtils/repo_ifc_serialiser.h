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
*		away when the IfcSchema definition changes. Use them only in the .cpp.
*/

#include "repo_ifc_serialiser_abstract.h"

#include "repo/manipulator/modelutility/repo_scene_builder.h"
#include "repo/core/model/bson/repo_node_transformation.h"
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
		unsigned int numThreads;
		repo::manipulator::modelutility::RepoSceneBuilder* builder;
		repo::lib::RepoUUID rootNodeId;
		std::unordered_map<std::string, repo::lib::repo_material_t> materials;

		/*
		* Cache entry representing the possible transformation nodes that result from
		* a single Ifc Object - either or both of the members can be set, indicating
		* whether the object has geometry, or children, or both.
		*/
		struct TransformationNodesInfo
		{
			repo::lib::RepoUUID branchSharedId;
			std::shared_ptr<repo::core::model::TransformationNode> leafNode;
		};

		std::unordered_map<int64_t, TransformationNodesInfo> sharedIds;
		std::unordered_map<int64_t, repo::lib::RepoUUID> metadataUniqueIds;

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

		void setLevelOfDetail(int lod);

		void setNumThreads(int numThreads);

		repo::lib::RepoBounds getBounds();

		const repo::lib::repo_material_t& resolveMaterial(
			const ifcopenshell::geometry::taxonomy::style::ptr ptr);

		/*
		* Helper type that represents an Ifc parameter that may come from a property,
		* attribute or another source, in its final form ready to be added to a
		* metadata node.
		*/

		struct RepoValue {
			std::optional<repo::lib::RepoVariant> v;
			std::string units;
		};

		/*
		* Helper class to build metadata entries, including a persistent context for
		* grouping nested entries.
		*
		* In most recursive calls of collectMetadata, the helper can be passed by
		* reference as the context will not change. When it does (e.g. grouping by
		* PSet), one of the methods can be used to create a copy with the updated
		* state to pass to that recursive branch.
		*/

		struct Metadata
		{
		private:
			std::unordered_map<std::string, repo::lib::RepoVariant>& metadata;
			std::string prefix;
			std::string suffix;
			bool overwrite;

		public:
			Metadata(std::unordered_map<std::string, repo::lib::RepoVariant>& map)
				:metadata(map),
				overwrite(true)
			{
			}

			Metadata addSuffix(const std::string& suffix) const {
				auto copy = *this;
				if (!copy.suffix.empty()) {
					copy.suffix = this->suffix + ":" + suffix;
				}
				else {
					copy.suffix = suffix;
				}
				return copy;
			}

			Metadata setGroup(const std::string& group) const {
				auto copy = *this;
				copy.prefix = group;
				return copy;
			}

			Metadata setOverwrite(bool overwrite) const {
				auto copy = *this;
				copy.overwrite = overwrite;
				return copy;
			}

			repo::lib::RepoVariant& operator()(const std::string& name)
			{
				return metadata[getKey(name, {})];
			}

			void setValue(const std::string& name, RepoValue value)
			{
				if (value.v) {
					auto key = getKey(name, value.units);
					if (!overwrite && metadata.find(key) != metadata.end()) {
						return;
					}
					else {
						metadata[key] = *value.v;
					}
				}
			}

		private:
			std::string getKey(const std::string& name, const std::string& units)
			{
				std::string fullName;
				if (!prefix.empty()) {
					fullName = prefix + "::" + name;
				}
				else {
					fullName = name;
				}
				if (!suffix.empty()) {
					fullName += " (" + suffix + ")";
				}
				if (!units.empty()) {
					fullName += " (" + units + ")";
				}
				return fullName;
			}
		};

		std::string getUnitsLabel(const IfcSchema::IfcUnit* unit);
		std::string getUnitsLabel(const IfcSchema::IfcSIUnitName::Value& name);
		std::string getUnitsLabel(const IfcSchema::IfcSIPrefix::Value& prefix);
		std::string getUnitsLabel(const std::string& name);
		std::string getUnitsLabel(const unit_t& units);
		std::string getUnitsLabel(const IfcParse::declaration& type);

		std::string getExponentAsString(int value);

		/*
		* If the base units label already contains the exponents, as will be the case
		* for area and volume, remove the exponent from the string and return it so it
		* can be recombined in derived units.
		*/
		int removeExponentFromString(std::string& base);

		/* Returns the unit of the first Measure Value in the list (if any) */

		std::string getUnitsLabel(const aggregate_of<IfcSchema::IfcValue>::ptr list);
		std::string getUnitsLabel(const boost::optional<aggregate_of<IfcSchema::IfcValue>::ptr> list);

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
		* Creates the associations between IFC Types and Unit Enums that are expressed
		* in the IfcMeasureResource, but are not programmatically linked in IFCOS. Should
		* be called early on (e.g. in the constructor).
		*/
		void setNamedTypeUnits();

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
		* leafNode is used to indicate that the new node holds only mesh nodes (to
		* start with).
		*/
		repo::lib::RepoUUID getTransformationNode(
			const IfcSchema::IfcObjectDefinition* object,
			bool leafNode);

		/*
		* Creates a transformation node under the parent object to group together
		* entities of a given type.
		*/
		repo::lib::RepoUUID getTransformationNode(
			const IfcSchema::IfcObjectDefinition* parent,
			const IfcParse::entity& type);

		std::shared_ptr<repo::core::model::TransformationNode> createTransformationNode(
			const IfcSchema::IfcObjectDefinition* object,
			const repo::lib::RepoUUID& parentId);

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

		/*
		* Given a value (value) from a type (type), extract the value into a
		* RepoVariant, while performing any post-processing, such as formatting,
		* appropriate to the type.
		*/
		RepoValue getValue(const IfcParse::declaration& type, const AttributeValue& value);

		RepoValue getValue(const IfcSchema::IfcValue* value);

		/*
		* Given an object reference, return a representation of that object that
		* can be used as a value in a key-value pair (instead of descending into
		* the object to collect its metadata as a sibling). In almost all cases
		* this will be some sort of string representation of its name(s) or Id.
		*/
		RepoValue getValue(const IfcSchema::IfcObjectReferenceSelect* object);

		/*
		* Return the list of IfcValues as a single variant. For lists with more than
		* one entry, the variant will be a string representation of the array.
		*/
		std::optional<repo::lib::RepoVariant> getValue(aggregate_of<IfcSchema::IfcValue>::ptr list);
		std::optional<repo::lib::RepoVariant> getValue(boost::optional<aggregate_of<IfcSchema::IfcValue>::ptr> list);

		void collectAdditionalAttributes(
			const IfcSchema::IfcObjectDefinition* object,
			Metadata& metadata);

		void collectAttributes(
			const IfcUtil::IfcBaseEntity* object,
			Metadata& metadata
		);

		/*
		* Recursively collects attributes and properties into the metadata object. This
		* method works by trying to convert object to one of a number of known types.
		* This method is intended to be used with both the underlying containers, such
		* as IfcProperty, and the relationship objects that compose sets of metadata,
		* such as ifcRelDefines or IfcRelNests.
		*/
		void collectMetadata(
			const IfcUtil::IfcBaseInterface* object,
			Metadata& metadata
		);

		template<typename T>
		void collectMetadata(
			T begin,
			T end,
			Metadata& metadata
		);

		bool shouldGroupByType(
			const IfcSchema::IfcObjectDefinition* object,
			const IfcSchema::IfcObjectDefinition* parent);
	};
}