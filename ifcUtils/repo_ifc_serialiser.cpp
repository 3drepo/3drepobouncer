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

#include "repo_ifc_serialiser.h"
#include "repo_ifc_constants.h"

#include <repo/repo_bouncer_global.h>
#include <repo/lib/repo_utils.h>
#include <repo/lib/datastructure/repo_vector.h>
#include <repo/lib/datastructure/repo_matrix.h>
#include <repo/lib/datastructure/repo_structs.h>
#include <repo/lib/datastructure/repo_variant.h>
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/error_codes.h"

#include <ifcparse/IfcEntityInstanceData.h>

#pragma optimize("",off)

using namespace ifcUtils;

/*
* The starting index of the subtype attributes - this is the same for Ifc2x3 and
* 4x, however if it changes in future versions we may have to get it from the
* schema instead.
*/
#define NUM_ROOT_ATTRIBUTES 4

static repo::lib::RepoVector3D64 repoVector(const ifcopenshell::geometry::taxonomy::point3& p)
{
	return repo::lib::RepoVector3D64(p.components_->x(), p.components_->y(), p.components_->z());
}

static repo::lib::repo_color3d_t repoColor(const ifcopenshell::geometry::taxonomy::colour& c)
{
	return repo::lib::repo_color3d_t(c.r(), c.g(), c.b());
}

static repo::lib::RepoMatrix repoMatrix(const ifcopenshell::geometry::taxonomy::matrix4& m)
{
	return repo::lib::RepoMatrix(m.ccomponents().data(), false);
}

template<typename T>
static std::string stringifyVector(std::vector<T> arr)
{
	std::stringstream ss;
	ss << "(";
	for (size_t i = 0; i < arr.size(); i++) {
		ss << arr[i];
		if (i < arr.size() - 1) {
			ss << ",";
		}
	}
	ss << ")";
	return ss.str();
}

template<typename T>
static repo::lib::RepoVariant stringify(std::vector<T> arr)
{
	repo::lib::RepoVariant v;
	if (arr.size()) {
		v = stringifyVector(arr);
	}
	return v;
}

template<typename T>
static repo::lib::RepoVariant stringify(std::vector<std::vector<T>> arr)
{
	repo::lib::RepoVariant v;
	if (arr.size()) {
		std::stringstream ss;
		ss << "(";
		for (size_t i = 0; i < arr.size(); i++) {
			ss << stringifyVector(arr[i]);
			if (i < arr.size() - 1) {
				ss << ",";
			}
		}
		ss << ")";
		v = ss.str();
	}
	return v;
}

static std::optional<repo::lib::RepoVariant> repoVariant(const AttributeValue& a)
{
	repo::lib::RepoVariant v;
	switch (a.type())
	{
	case IfcUtil::ArgumentType::Argument_NULL:
	case IfcUtil::ArgumentType::Argument_DERIVED: // not clear what this one is...
		return std::nullopt;
	case IfcUtil::ArgumentType::Argument_INT:
		v = a.operator int();
		return v;
	case IfcUtil::ArgumentType::Argument_BOOL:
		v = a.operator bool();
		return v;
	case IfcUtil::ArgumentType::Argument_LOGICAL:
	{
		auto value = a.operator boost::logic::tribool();
		if (value == boost::logic::tribool::true_value) {
			v = true;
		}
		else if (value == boost::logic::tribool::false_value) {
			v = false;
		}
		else {
			v = "UNKNOWN";
		}
		return v;
	}
	case IfcUtil::ArgumentType::Argument_DOUBLE:
		v = a.operator double();
		return v;
	case IfcUtil::ArgumentType::Argument_STRING:
		v = a.operator std::string();
		return v;

	// Aggregations will often hold things such as GIS coordinates; usually these
	// complex types should be formatted explicitly by processAttributeValue, but
	// make a best effort to return something as a fallback if not.

	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_INT:
		v = stringify(a.operator std::vector<int, std::allocator<int>>());
		return v;
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_DOUBLE:
		v = stringify(a.operator std::vector<double, std::allocator<double>>());
		return v;
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_STRING:
		v = stringify(a.operator std::vector<std::string, std::allocator<std::string>>());
		return v;
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_INT:
		v = stringify(a.operator std::vector<std::vector<int, std::allocator<int>>, std::allocator<std::vector<int, std::allocator<int>>>>());
		return v;
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_DOUBLE:
		v = stringify(a.operator std::vector<std::vector<double, std::allocator<double>>, std::allocator<std::vector<double, std::allocator<double>>>>());
		return v;

	// IFCOS will conveniently resolve the enumeration label through the string
	// conversion without us having to do anything else...

	case IfcUtil::ArgumentType::Argument_ENUMERATION:
		v = a.operator std::string();
		return v;

	case IfcUtil::ArgumentType::Argument_BINARY:
	case IfcUtil::ArgumentType::Argument_ENTITY_INSTANCE:
	case IfcUtil::ArgumentType::Argument_EMPTY_AGGREGATE:
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_BINARY:
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_ENTITY_INSTANCE:
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_EMPTY_AGGREGATE:
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE:
	case IfcUtil::ArgumentType::Argument_UNKNOWN:
		return std::nullopt;
	}
}

static std::string constructMetadataLabel(
	const std::string& label,
	const std::string& prefix,
	const std::string& units = {}
) {
	std::stringstream ss;

	if (!prefix.empty())
		ss << prefix << "::";

	ss << label;

	if (!units.empty()) {
		ss << " (" << units << ")";
	}
	return ss.str();
}

static std::string getCurrencyLabel(const std::string& code)
{
	return code;
}

#ifdef SCHEMA_HAS_IfcCurrencyEnum
static std::string getCurrencyLabel(const IfcSchema::IfcCurrencyEnum::Value& code)
{
	return IfcSchema::IfcCurrencyEnum::ToString(code);
}
#endif

std::string IfcSerialiser::getExponentAsString(int value)
{
	if (value == 1)
	{
		return {};
	}

	auto valueStr = std::to_string(value);
	std::stringstream ss;
	std::string symbolMapping[] = {
		"⁰",
		"¹",
		"²",
		"³",
		"⁴",
		"⁵",
		"⁶",
		"⁷",
		"⁸",
		"⁹"
	};

	for (const auto& chr : valueStr) {
		if (chr == '-') {
			ss << "⁻";
		}
		else {
			int digit = atoi(&chr);
			ss << symbolMapping[digit];
		}
	}

	return ss.str();
}

std::string IfcSerialiser::getUnitsLabel(const IfcSchema::IfcSIUnitName::Value& name)
{
	switch (name)
	{
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_AMPERE:
		return "A";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_BECQUEREL:
		return "Bq";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_CANDELA:
		return "cd";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_COULOMB:
		return "C";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_CUBIC_METRE:
		return "m³";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_DEGREE_CELSIUS:
		return "°C";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_FARAD:
		return "F";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_GRAM:
		return "g";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_GRAY:
		return "Gy";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_HENRY:
		return "H";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_HERTZ:
		return "Hz";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_JOULE:
		return "J";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_KELVIN:
		return "K";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_LUMEN:
		return "lm";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_LUX:
		return "lx";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_METRE:
		return "m";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_MOLE:
		return "mol";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_NEWTON:
		return "N";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_OHM:
		return "Ω";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_PASCAL:
		return "Pa";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_RADIAN:
		return "rad";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_SECOND:
		return "s";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_SIEMENS:
		return "S";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_SIEVERT:
		return "Sv";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_SQUARE_METRE:
		return "m²";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_STERADIAN:
		return "sr";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_TESLA:
		return "T";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_VOLT:
		return "V";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_WATT:
		return "W";
	case IfcSchema::IfcSIUnitName::Value::IfcSIUnitName_WEBER:
		return "Wb";
	}

	return "";
}

std::string IfcSerialiser::getUnitsLabel(const IfcSchema::IfcSIPrefix::Value& prefix)
{
	switch (prefix)
	{
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_EXA:
		return "E";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_PETA:
		return "P";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_TERA:
		return "T";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_GIGA:
		return "G";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_MEGA:
		return "M";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_KILO:
		return "k";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_HECTO:
		return "h";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_DECA:
		return "da";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_DECI:
		return "d";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_CENTI:
		return "c";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_MILLI:
		return "m";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_MICRO:
		return "µ";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_NANO:
		return "n";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_PICO:
		return "p";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_FEMTO:
		return "f";
	case IfcSchema::IfcSIPrefix::Value::IfcSIPrefix_ATTO:
		return "a";
	}

	return "";
}

std::string IfcSerialiser::getUnitsLabel(const std::string& unitName)
{
	std::string data = unitName;
	repo::lib::toLower(data);

	// Some conversion unit descriptions are identical to the units. We only check
	// below for those we have known symbols for.

	if (data == "inch") return "in";
	if (data == "foot") return "ft";
	if (data == "yard") return "yd";
	if (data == "mile") return "mi";
	if (data == "acre") return "ac";
	if (data == "square inch") return "in²";
	if (data == "square foot") return "ft²";
	if (data == "square yard") return "yd²";
	if (data == "square mile") return "mi²";
	if (data == "cubie inch") return "in³";
	if (data == "cubic foot") return "ft³";
	if (data == "cubic yard") return "yd³";
	if (data == "litre") return "l";
	if (data == "fluid ounce uk") return "fl oz(UK)";
	if (data == "fluid ounce us") return "fl oz(US)";
	if (data == "pint uk") return "pint(UK)";
	if (data == "pint us") return "pint(UK)";
	if (data == "gallon uk") return "gal(UK)";
	if (data == "gallon us") return "gal(UK)";
	if (data == "degree") return "°";
	if (data == "ounce") return "oz";
	if (data == "pound") return "lb";
	if (data == "ton uk") return "t(UK)";
	if (data == "ton us") return "t(US)";
	if (data == "minute") return "mins";
	if (data == "hour") return "hr";
	if (data == "day") return "day";
	if (data == "btu") return "btu";

	return data;
}

std::string IfcSerialiser::getUnitsLabel(const IfcSchema::IfcUnit* unit)
{
	if (auto u = unit->as<IfcSchema::IfcSIUnit>())
	{
		auto base = getUnitsLabel(u->Name());
		auto prefix = u->Prefix() ? getUnitsLabel(*u->Prefix()) : "";
		return prefix + base;
	}
	else if (auto u = unit->as<IfcSchema::IfcContextDependentUnit>())
	{
		return getUnitsLabel(u->Name());
	}
	else if (auto u = unit->as<IfcSchema::IfcConversionBasedUnit>())
	{
		return getUnitsLabel(u->Name());
	}
	else if (auto u = unit->as<IfcSchema::IfcMonetaryUnit>())
	{
		return getCurrencyLabel(u->Currency());
	}
	else if (auto u = unit->as<IfcSchema::IfcDerivedUnit>())
	{
		std::stringstream ss;
		auto elements = u->Elements();
		for (const auto& e : *elements)
		{
			auto base = getUnitsLabel(e->Unit());
			if (!base.empty()) {
				ss << base << getExponentAsString(e->Exponent());
			}
			else
			{
				std::stringstream es;
				e->Unit()->data().toString(es);
				repoWarning << "Unrecognised Derived Unit Element " << es.str();
			}
		}
		return ss.str();
	}
	else if (auto u = unit->as<IfcSchema::IfcNamedUnit>()) // Prefer the subtype interpretation, but use the base class as a fallback
	{
		std::stringstream ss;
		auto dimensions = u->Dimensions();
		if (dimensions->LengthExponent() != 0) {
			ss << getUnitsLabel(IfcSchema::IfcUnitEnum::Value::IfcUnit_LENGTHUNIT) << getExponentAsString(dimensions->LengthExponent());
		}
		if (dimensions->MassExponent() != 0) {
			ss << getUnitsLabel(IfcSchema::IfcUnitEnum::Value::IfcUnit_MASSUNIT) << getExponentAsString(dimensions->MassExponent());
		}
		if (dimensions->TimeExponent() != 0) {
			ss << getUnitsLabel(IfcSchema::IfcUnitEnum::Value::IfcUnit_TIMEUNIT) << getExponentAsString(dimensions->TimeExponent());
		}
		if (dimensions->ElectricCurrentExponent() != 0) {
			ss << getUnitsLabel(IfcSchema::IfcUnitEnum::Value::IfcUnit_ELECTRICCURRENTUNIT) << getExponentAsString(dimensions->ElectricCurrentExponent());
		}
		if (dimensions->ThermodynamicTemperatureExponent() != 0) {
			ss << getUnitsLabel(IfcSchema::IfcUnitEnum::Value::IfcUnit_THERMODYNAMICTEMPERATUREUNIT) << getExponentAsString(dimensions->ThermodynamicTemperatureExponent());
		}
		if (dimensions->AmountOfSubstanceExponent() != 0) {
			ss << getUnitsLabel(IfcSchema::IfcUnitEnum::Value::IfcUnit_AMOUNTOFSUBSTANCEUNIT) << getExponentAsString(dimensions->AmountOfSubstanceExponent());
		}
		if (dimensions->LuminousIntensityExponent() != 0) {
			ss << getUnitsLabel(IfcSchema::IfcUnitEnum::Value::IfcUnit_LUMINOUSINTENSITYUNIT) << getExponentAsString(dimensions->LuminousIntensityExponent());
		}
		return ss.str();
	}

	return {};
}

std::string IfcSerialiser::getUnitsLabel(const IfcParse::declaration& type)
{
	auto c = definedValueTypeUnits.find(type.index_in_schema());
	if (c != definedValueTypeUnits.end())
	{
		return getUnitsLabel(c->second);
	}
	return "";
}

std::string IfcSerialiser::getUnitsLabel(const unit_t& unit)
{
	auto label = unitLabels.find(unit);
	if (label != unitLabels.end()) {
		return label->second;
	}
	return "";
}

void IfcSerialiser::setUnitsLabel(unit_t unit, const IfcSchema::IfcUnit* definition)
{
	unitLabels[unit] = getUnitsLabel(definition);
}

IfcSerialiser::IfcSerialiser(std::unique_ptr<IfcParse::IfcFile> file)
	:file(std::move(file))
{
	kernel = "opencascade";
	numThreads = std::thread::hardware_concurrency();
	setDefinedTypeUnits();
	configureSettings();
}

void IfcSerialiser::configureSettings()
{
	// Look at the ConversionSettings.h file, or the Python documentation, for the
	// set of options. Here we override only those that have a different default
	// value to what we want.

	settings.get<ifcopenshell::geometry::settings::WeldVertices>().value = false; // Welding vertices can make uv projection difficult
	settings.get<ifcopenshell::geometry::settings::GenerateUvs>().value = true;
}

void IfcSerialiser::updateBounds()
{
	auto bounds = getBounds();
	builder->setWorldOffset(bounds.min());
	settings.get<ifcopenshell::geometry::settings::ModelOffset>().value = (-builder->getWorldOffset()).toStdVector();
}

repo::lib::RepoBounds IfcSerialiser::getBounds()
{
	IfcGeom::Iterator boundsIterator(kernel, settings, file.get(), {}, numThreads);
	boundsIterator.initialize();
	boundsIterator.compute_bounds(false); // False here means the geometry is not triangulated but we get more approximate bounds based on the transforms
	repo::lib::RepoBounds bounds(repoVector(boundsIterator.bounds_min()), repoVector(boundsIterator.bounds_max()));
	return bounds;
}

const repo::lib::repo_material_t& IfcSerialiser::resolveMaterial(const ifcopenshell::geometry::taxonomy::style::ptr ptr)
{
	auto name = ptr->name;
	IfcUtil::sanitate_material_name(name);

	if (materials.find(name) == materials.end())
	{
		repo::lib::repo_material_t material = repo::lib::repo_material_t::DefaultMaterial();

		material.diffuse = repoColor(ptr->get_color());

		material.specular = repoColor(ptr->specular);
		if (ptr->has_specularity())
		{
			material.shininess = ptr->specularity;
		}
		else
		{
			material.shininess = 0.5f;
		}
		material.shininessStrength = 0.5f;

		if (ptr->has_transparency()) {
			material.opacity = 1 - ptr->transparency;
		}

		materials[name] = material;
	}

	return materials[name];
}

void IfcSerialiser::createRootNode()
{
	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	builder->addNode(rootNode);
	rootNodeId = rootNode.getSharedID();
}

/*
* Given an arbitrary Ifc Object, determine from its relationships which is the
* best one for it to sit under in the acyclic tree.
*/
const IfcSchema::IfcObjectDefinition* IfcSerialiser::getParent(const IfcSchema::IfcObjectDefinition* object)
{
	auto connects = object->file_->getInverse(object->id(), &(IfcSchema::IfcRelContainedInSpatialStructure::Class()), -1);
	for (auto& rel : *connects) {
		auto contained = rel->as<IfcSchema::IfcRelContainedInSpatialStructure>();
		auto structure = contained->RelatingStructure();
		if (!structure || structure == object) {
			continue;
		}
		return structure;
	}

	auto compositions = object->file_->getInverse(object->id(), &(IfcSchema::IfcRelDecomposes::Class()), -1);
	for (auto& composition : *compositions) {
		if(auto c = composition->as<IfcSchema::IfcRelNests>()){
			if (auto p = c->RelatingObject()) {
				if (p && p != object) {
					return p;
				}
			}
		}

		if (auto c = composition->as<IfcSchema::IfcRelAggregates>()) {
			if (auto p = c->RelatingObject()) {
				if (p && p != object) {
					return p;
				}
			}
		}

		// We may also consider IfcRelVoidsElement here, but we don't import openings
		// at all currently.
	}

	return nullptr;
}

repo::lib::RepoUUID IfcSerialiser::createTransformationNode(const IfcSchema::IfcObjectDefinition* parent, const IfcParse::entity& type)
{
	auto parentId = createTransformationNode(parent);

	auto groupId = parent->file_->getMaxId() + parent->id() + type.type() + 1;
	if (sharedIds.find(groupId) != sharedIds.end()) {
		return sharedIds[groupId];
	}

	auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode({}, type.name(), { parentId });
	sharedIds[groupId] = transform.getSharedID();;
	builder->addNode(transform);

	return transform.getSharedID();
}

/*
* If this entity sit under an empty node in the tree that doesn't exist in
* the Ifc, but exists in the Repo tree to group entities by type.
*/
bool IfcSerialiser::shouldGroupByType(const IfcSchema::IfcObjectDefinition* object, const typename IfcSchema::IfcObjectDefinition* parent)
{
	return parent && !parent->as<IfcSchema::IfcElement>() && object->as<IfcSchema::IfcElement>();
}

repo::lib::RepoUUID IfcSerialiser::createTransformationNode(const IfcSchema::IfcObjectDefinition* object)
{
	if (!object)
	{
		return rootNodeId;
	}

	// If this object already has a transformation node, return its Id. An
	// existing node will already have all its metdata set up, etc.

	if (sharedIds.find(object->id()) != sharedIds.end())
	{
		return sharedIds[object->id()];
	}

	auto parent = getParent(object);

	auto parentId = createTransformationNode(parent);

	// If the parent is a non-physical container, group elements by type, as
	// a convenience.

	if (shouldGroupByType(object, parent))
	{
		parentId = createTransformationNode(parent, object->declaration());
	}

	auto name = object->Name().get_value_or({});
	if (name.empty())
	{
		name = object->declaration().name();
	}

	auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode({}, name, { parentId });
	parentId = transform.getSharedID();
	sharedIds[object->id()] = parentId;
	builder->addNode(transform);

	auto metaNode = createMetadataNode(object);
	metaNode->addParent(parentId);
	builder->addNode(std::move(metaNode));

	return parentId;
}

repo::lib::RepoUUID IfcSerialiser::getParentId(const IfcGeom::Element* element, bool createTransform)
{
	auto object = element->product()->as<IfcSchema::IfcObjectDefinition>();
	if (createTransform)
	{
		return createTransformationNode(object);
	}
	else
	{
		auto parent = getParent(object);
		if (shouldGroupByType(object, parent))
		{
			return createTransformationNode(parent, object->declaration());
		}
		else
		{
			return createTransformationNode(parent);
		}
	}
}

void IfcSerialiser::import(const IfcGeom::TriangulationElement* triangulation)
{
	auto& mesh = triangulation->geometry();

	auto matrix = repoMatrix(*triangulation->transformation().data());

	std::vector<repo::lib::RepoVector3D> vertices;
	vertices.reserve(mesh.verts().size() / 3);
	for (auto it = mesh.verts().begin(); it != mesh.verts().end();) {
		const float x = *(it++);
		const float y = *(it++);
		const float z = *(it++);
		vertices.push_back({ x, y, z });
	}

	std::vector<repo::lib::RepoVector3D> normals;
	normals.reserve(mesh.normals().size() / 3);
	for (auto it = mesh.normals().begin(); it != mesh.normals().end();) {
		const float x = *(it++);
		const float y = *(it++);
		const float z = *(it++);
		normals.push_back({ x, y, z });
	}

	std::vector<repo::lib::RepoVector2D> uvs;
	normals.reserve(mesh.uvs().size() / 2);
	for (auto it = mesh.uvs().begin(); it != mesh.uvs().end();) {
		const float u = *it++;
		const float v = *it++;
		uvs.push_back({ u, v });
	}

	auto& facesIt = mesh.faces();
	auto facesMaterialIt = mesh.material_ids().begin();
	auto materials = mesh.materials();

	std::unordered_map<size_t, std::vector<repo::lib::repo_face_t>> facesByMaterial;
	for (auto it = facesIt.begin(); it != facesIt.end();)
	{
		const int materialId = *(facesMaterialIt++);
		auto& faces = facesByMaterial[materialId];
		const size_t v1 = *(it++);
		const size_t v2 = *(it++);
		const size_t v3 = *(it++);
		faces.push_back({ v1, v2, v3 });
	}

	if (!facesByMaterial.size()) {
		return;
	}

	bool createTransformNode = facesByMaterial.size() > 1;

	std::string name;
	if (!createTransformNode) {
		name = triangulation->name();
	}

	if (triangulation->product()->as<IfcSchema::IfcSpace>()) {
		name += " (IFC Space)";
	}

	auto parentId = getParentId(triangulation, createTransformNode);

	std::unique_ptr<repo::core::model::MetadataNode> metaNode;
	if (!createTransformNode) {
		metaNode = createMetadataNode(triangulation->product()->as<IfcSchema::IfcObjectDefinition>());
	}

	for (auto pair : facesByMaterial)
	{
		// Vertices that are not referenced by a particular set of faces will be
		// removed when removeDuplicates is called as part of the commit process.

		auto mesh = repo::core::model::RepoBSONFactory::makeMeshNode(
			vertices,
			pair.second,
			normals,
			{},
			{ uvs },
			name,
			{ parentId }
		);
		mesh.applyTransformation(matrix);
		mesh.updateBoundingBox();
		builder->addNode(mesh);
		builder->addMaterialReference(resolveMaterial(materials[pair.first]), mesh.getSharedID());

		if (metaNode) {
			metaNode->addParent(mesh.getSharedID());
		}
	}

	if (metaNode) {
		builder->addNode(std::move(metaNode));
	}
}

void collectMetadata(const IfcSchema::IfcObjectDefinition* object, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata, std::string group)
{
	if (auto propVal = dynamic_cast<const IfcSchema::IfcPropertySingleValue*>(object)) {
		/*
		std::string value = "n/a";
		std::string units = "";
		if (propVal->NominalValue())
		{
			value = getValueAsString(propVal->NominalValue(), units, projectUnits);
		}

		if (propVal->Unit())
		{
			units = processUnits(propVal->Unit()).second;
		}

		metaValues[constructMetadataLabel(propVal->Name(), metaPrefix, units)] = value;
		*/
	}

	if (auto defByProp = dynamic_cast<const IfcSchema::IfcRelDefinesByProperties*>(object)) {
		/*
		extraChildren.push_back(defByProp->RelatingPropertyDefinition());
		action.cacheMetadata = true;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto propSet = dynamic_cast<const IfcSchema::IfcPropertySet*>(object)) {
		/*
		auto propDefs = propSet->HasProperties();
		extraChildren.insert(extraChildren.end(), propDefs->begin(), propDefs->end());
		childrenMetaPrefix = constructMetadataLabel(propSet->Name(), metaPrefix);

		action.createElement = false;
		action.cacheMetadata = true;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto propSet = dynamic_cast<const IfcSchema::IfcPropertyBoundedValue*>(object)) {
		/*
		auto unitsOverride = propSet->hasUnit() ? processUnits(propSet->Unit()).second : "";
		std::string upperBound, lowerBound;
		if (propSet->hasUpperBoundValue()) {
			std::string units;
			upperBound = getValueAsString(propSet->UpperBoundValue(), units, projectUnits);
			if (unitsOverride.empty()) unitsOverride = units;
		}
		if (propSet->hasLowerBoundValue()) {
			std::string units;
			lowerBound = getValueAsString(propSet->UpperBoundValue(), units, projectUnits);
			if (unitsOverride.empty()) unitsOverride = units;
		}

		metaValues[constructMetadataLabel(propSet->Name(), metaPrefix, unitsOverride)] = "[" + lowerBound + ", " + upperBound + "]";

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto eleQuan = dynamic_cast<const IfcSchema::IfcElementQuantity*>(object)) {
		/*
		auto quantities = eleQuan->Quantities();
		extraChildren.insert(extraChildren.end(), quantities->begin(), quantities->end());
		childrenMetaPrefix = constructMetadataLabel(eleQuan->Name(), metaPrefix);

		action.createElement = false;
		action.takeRefsAsChildren = false;
		action.cacheMetadata = true;
		*/
	}

	if (auto quantities = dynamic_cast<const IfcSchema::IfcPhysicalComplexQuantity*>(object)) {
		/*
		childrenMetaPrefix = constructMetadataLabel(quantities->Name(), metaPrefix);
		auto quantitySet = quantities->HasQuantities();
		extraChildren.insert(extraChildren.end(), quantitySet->begin(), quantitySet->end());

		action.createElement = false;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityLength*>(object)) {
		/*
		const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second :
			getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LENGTHUNIT, projectUnits);

		metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->LengthValue();

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityArea*>(object)) {
		/*
		const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_AREAUNIT, projectUnits);

		metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->AreaValue();

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityCount*>(object)) {
		/*
		const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : "";
		metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->CountValue();

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityTime*>(object)) {
		/*
		const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_TIMEUNIT, projectUnits);
		metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->TimeValue();

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityVolume*>(object)) {
		/*
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_VOLUMEUNIT, projectUnits);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->VolumeValue();

			action.createElement = false;
			action.traverseChildren = false;
			*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityWeight*>(object)) {
		/*
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MASSUNIT, projectUnits);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->WeightValue();

			action.createElement = false;
			action.traverseChildren = false;
			*/
	}

	if (auto relCS = dynamic_cast<const IfcSchema::IfcRelAssociatesClassification*>(object)) {
		/*
		action.traverseChildren = false;
		action.takeRefsAsChildren = false;
		generateClassificationInformation(relCS, metaValues);
		*/
	}

	if (auto relCS = dynamic_cast<const IfcSchema::IfcRelContainedInSpatialStructure*>(object)) {
		/*
		try {
			auto relatedObjects = relCS->RelatedElements();
			if (relatedObjects)
			{
				extraChildren.insert(extraChildren.end(), relatedObjects->begin(), relatedObjects->end());
			}
		}
		catch (const IfcParse::IfcException& e)
		{
			repoError << "Failed to retrieve related elements from " << relCS->data().id() << ": " << e.what();
			missingEntities = true;
		}
		action.createElement = false;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto relAgg = dynamic_cast<const IfcSchema::IfcRelAggregates*>(object)) {
		/*
		auto relatedObjects = relAgg->RelatedObjects();
		if (relatedObjects)
		{
			extraChildren.insert(extraChildren.end(), relatedObjects->begin(), relatedObjects->end());
		}
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto eleType = dynamic_cast<const IfcSchema::IfcTypeObject*>(object)) {
		/*
		if (eleType->hasHasPropertySets()) {
			auto propSets = eleType->HasPropertySets();
			extraChildren.insert(extraChildren.end(), propSets->begin(), propSets->end());
		}

		action.createElement = false;
		action.traverseChildren = true;
		action.cacheMetadata = true;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto eleType = dynamic_cast<const IfcSchema::IfcPropertyDefinition*>(object)) {
		/*
		action.createElement = false;
		action.traverseChildren = false;
		action.takeRefsAsChildren = false;
		*/
	}
}

/*
For some types, build explicit metadata entries
*/
void IfcSerialiser::collectAdditionalAttributes(const IfcSchema::IfcObjectDefinition* object, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (auto project = dynamic_cast<const IfcSchema::IfcProject*>(object)) {
		metadata[constructMetadataLabel(PROJECT_LABEL, LOCATION_LABEL)] = project->Name().get_value_or("(" + object->declaration().name() + ")");
	}

	if (auto building = dynamic_cast<const IfcSchema::IfcBuilding*>(object)) {
		metadata[constructMetadataLabel(BUILDING_LABEL, LOCATION_LABEL)] = building->Name().get_value_or("(" + object->declaration().name() + ")");
	}

	if (auto storey = dynamic_cast<const IfcSchema::IfcBuildingStorey*>(object)) {
		metadata[constructMetadataLabel(STOREY_LABEL, LOCATION_LABEL)] = storey->Name().get_value_or("(" + object->declaration().name() + ")");
	}
}

IfcSerialiser::Formatted IfcSerialiser::processAttributeValue(const IfcParse::attribute* attribute, const AttributeValue& value, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	// Ifc types provide information on how to interpret the underlying values; we
	// can use these to determine units, and explicit formatting where necessary.

	auto type = attribute->type_of_attribute();

	// GIS coordinates requires custom formatting that is not encompassed in the
	// units or anywhere programmatically

	if (type->is(IfcSchema::IfcCompoundPlaneAngleMeasure::Class())) {
		auto c = value.operator std::vector<int, std::allocator<int>>();
		std::stringstream ss;
		ss << c[0] << "° ";
		ss << abs(c[1]) << "' ";
		ss << abs(c[2]) << "\" ";
		ss << abs(c[3]);
		repo::lib::RepoVariant v;
		v = ss.str();
		return { v, {} };
	}

	std::string units;
	if (auto named = type->as_named_type()) {
		units = getUnitsLabel(*named->declared_type());
	}

	return { repoVariant(value), units };
}

std::unique_ptr<repo::core::model::MetadataNode> IfcSerialiser::createMetadataNode(const IfcSchema::IfcObjectDefinition* object)
{
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;

	auto& type = object->declaration();
	auto& instance = object->data();

	metadata[REPO_LABEL_IFC_TYPE] = type.name();
	metadata[REPO_LABEL_IFC_NAME] = object->Name().get_value_or("(" + type.name() + ")");
	metadata[REPO_LABEL_IFC_GUID] = object->GlobalId();
	if (object->Description()) {
		metadata[REPO_LABEL_IFC_DESCRIPTION] = object->Description().value();
	}

	// All types inherit from IfcRoot, which defines four attributes, of which we
	// take three explicitly above. The remaining attributes depend on the subtype.

	for (size_t i = NUM_ROOT_ATTRIBUTES; i < type.attribute_count(); i++)
	{
		auto attribute = type.attribute_by_index(i);
		auto value = object->data().get_attribute_value(i);
		
		if (!value.isNull()) {
			auto f = processAttributeValue(attribute, value, metadata);
			if (f.v) {
				auto name = attribute->name();
				if (!f.units.empty()) {
					name = name + " (" + f.units + ")";
				}
				metadata[name] = *f.v;
			}
		}
	}

	collectAdditionalAttributes(object, metadata);

	auto metadataNode = std::make_unique<repo::core::model::MetadataNode>(repo::core::model::RepoBSONFactory::makeMetaDataNode(metadata, {}, {}));
	return metadataNode;
}

void IfcSerialiser::setUnits(IfcSchema::IfcSIUnit u)
{
	unitLabels[u.UnitType()] = getUnitsLabel(&u);
}

void IfcSerialiser::setUnits(const IfcParse::type_declaration& type, unit_t units)
{
	definedValueTypeUnits[type.index_in_schema()] = units;
}

void IfcSerialiser::setDefinedTypeUnits()
{
	// Ifc defines a number of value and measurement types. As far as we are aware,
	// IFCOS has no programmatic mapping between these and the unit enums, so we
	// create them ourselves. The assignments can be see in the IfcMeasureResource
	// table, for example, for ifc4:
	// https://standards.buildingsmart.org/IFC/RELEASE/IFC4/ADD1/HTML/schema/ifcmeasureresource/content.htm

	setUnits(IfcSchema::IfcAbsorbedDoseMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_ABSORBEDDOSEUNIT);
	setUnits(IfcSchema::IfcRadioActivityMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_RADIOACTIVITYUNIT);
	setUnits(IfcSchema::IfcAmountOfSubstanceMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_AMOUNTOFSUBSTANCEUNIT);
	setUnits(IfcSchema::IfcAreaMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_AREAUNIT);
	setUnits(IfcSchema::IfcElectricCapacitanceMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_ELECTRICCAPACITANCEUNIT);
	setUnits(IfcSchema::IfcThermodynamicTemperatureMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_THERMODYNAMICTEMPERATUREUNIT);
	setUnits(IfcSchema::IfcDoseEquivalentMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_DOSEEQUIVALENTUNIT);
	setUnits(IfcSchema::IfcElectricChargeMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_ELECTRICCHARGEUNIT);
	setUnits(IfcSchema::IfcElectricConductanceMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_ELECTRICCONDUCTANCEUNIT);
	setUnits(IfcSchema::IfcElectricCurrentMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_ELECTRICCURRENTUNIT);
	setUnits(IfcSchema::IfcElectricVoltageMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_ELECTRICVOLTAGEUNIT);
	setUnits(IfcSchema::IfcElectricResistanceMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_ELECTRICRESISTANCEUNIT);
	setUnits(IfcSchema::IfcEnergyMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_ENERGYUNIT);
	setUnits(IfcSchema::IfcForceMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_FORCEUNIT);
	setUnits(IfcSchema::IfcFrequencyMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_FREQUENCYUNIT);
	setUnits(IfcSchema::IfcIlluminanceMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_ILLUMINANCEUNIT);
	setUnits(IfcSchema::IfcInductanceMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_INDUCTANCEUNIT);
	setUnits(IfcSchema::IfcLengthMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_LENGTHUNIT);
	setUnits(IfcSchema::IfcLuminousFluxMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_LUMINOUSFLUXUNIT);
	setUnits(IfcSchema::IfcLuminousIntensityMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_LUMINOUSINTENSITYUNIT);
	setUnits(IfcSchema::IfcMagneticFluxMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_MAGNETICFLUXUNIT);
	setUnits(IfcSchema::IfcMagneticFluxDensityMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_MAGNETICFLUXDENSITYUNIT);
	setUnits(IfcSchema::IfcMassMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_MASSUNIT);
	setUnits(IfcSchema::IfcPlaneAngleMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_PLANEANGLEUNIT);
	setUnits(IfcSchema::IfcPositiveLengthMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_LENGTHUNIT);
	setUnits(IfcSchema::IfcPositivePlaneAngleMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_PLANEANGLEUNIT);
	setUnits(IfcSchema::IfcPowerMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_POWERUNIT);
	setUnits(IfcSchema::IfcPressureMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_PRESSUREUNIT);
	setUnits(IfcSchema::IfcSolidAngleMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_SOLIDANGLEUNIT);
	setUnits(IfcSchema::IfcTimeMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_TIMEUNIT);
	setUnits(IfcSchema::IfcVolumeMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_VOLUMEUNIT);
	setUnits(IfcSchema::IfcAccelerationMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_ACCELERATIONUNIT);
	setUnits(IfcSchema::IfcPHMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_PHUNIT);
	setUnits(IfcSchema::IfcAngularVelocityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_ANGULARVELOCITYUNIT);
	setUnits(IfcSchema::IfcCompoundPlaneAngleMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_COMPOUNDPLANEANGLEUNIT);
	setUnits(IfcSchema::IfcCurvatureMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_CURVATUREUNIT);
	setUnits(IfcSchema::IfcDynamicViscosityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_DYNAMICVISCOSITYUNIT);
	setUnits(IfcSchema::IfcHeatFluxDensityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_HEATFLUXDENSITYUNIT);
	setUnits(IfcSchema::IfcHeatingValueMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_HEATINGVALUEUNIT);
	setUnits(IfcSchema::IfcIntegerCountRateMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_INTEGERCOUNTRATEUNIT);
	setUnits(IfcSchema::IfcIonConcentrationMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_IONCONCENTRATIONUNIT);
	setUnits(IfcSchema::IfcIsothermalMoistureCapacityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_ISOTHERMALMOISTURECAPACITYUNIT);
	setUnits(IfcSchema::IfcKinematicViscosityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_KINEMATICVISCOSITYUNIT);
	setUnits(IfcSchema::IfcLinearForceMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_LINEARFORCEUNIT);
	setUnits(IfcSchema::IfcLinearMomentMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_LINEARMOMENTUNIT);
	setUnits(IfcSchema::IfcLinearStiffnessMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_LINEARSTIFFNESSUNIT);
	setUnits(IfcSchema::IfcLinearVelocityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_LINEARVELOCITYUNIT);
	setUnits(IfcSchema::IfcLuminousIntensityDistributionMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_LUMINOUSINTENSITYDISTRIBUTIONUNIT);
	setUnits(IfcSchema::IfcMassDensityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MASSDENSITYUNIT);
	setUnits(IfcSchema::IfcMassFlowRateMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MASSFLOWRATEUNIT);
	setUnits(IfcSchema::IfcMassPerLengthMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MASSPERLENGTHUNIT);
	setUnits(IfcSchema::IfcModulusOfElasticityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MODULUSOFELASTICITYUNIT);
	setUnits(IfcSchema::IfcModulusOfLinearSubgradeReactionMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MODULUSOFLINEARSUBGRADEREACTIONUNIT);
	setUnits(IfcSchema::IfcModulusOfRotationalSubgradeReactionMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MODULUSOFROTATIONALSUBGRADEREACTIONUNIT);
	setUnits(IfcSchema::IfcModulusOfSubgradeReactionMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MODULUSOFSUBGRADEREACTIONUNIT);
	setUnits(IfcSchema::IfcMoistureDiffusivityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MOISTUREDIFFUSIVITYUNIT);
	setUnits(IfcSchema::IfcMolecularWeightMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MOLECULARWEIGHTUNIT);
	setUnits(IfcSchema::IfcMomentOfInertiaMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_MOMENTOFINERTIAUNIT);
	setUnits(IfcSchema::IfcPlanarForceMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_PLANARFORCEUNIT);
	setUnits(IfcSchema::IfcRotationalFrequencyMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_ROTATIONALFREQUENCYUNIT);
	setUnits(IfcSchema::IfcRotationalMassMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_ROTATIONALMASSUNIT);
	setUnits(IfcSchema::IfcRotationalStiffnessMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_ROTATIONALSTIFFNESSUNIT);
	setUnits(IfcSchema::IfcSectionalAreaIntegralMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_SECTIONAREAINTEGRALUNIT);
	setUnits(IfcSchema::IfcSectionModulusMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_SECTIONMODULUSUNIT);
	setUnits(IfcSchema::IfcShearModulusMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_SHEARMODULUSUNIT);
	setUnits(IfcSchema::IfcSoundPowerMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_SOUNDPOWERUNIT);
	setUnits(IfcSchema::IfcSoundPressureMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_SOUNDPRESSUREUNIT);
	setUnits(IfcSchema::IfcSpecificHeatCapacityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_SPECIFICHEATCAPACITYUNIT);
	setUnits(IfcSchema::IfcTemperatureGradientMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_TEMPERATUREGRADIENTUNIT);
	setUnits(IfcSchema::IfcThermalAdmittanceMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_THERMALADMITTANCEUNIT);
	setUnits(IfcSchema::IfcThermalExpansionCoefficientMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_THERMALEXPANSIONCOEFFICIENTUNIT);
	setUnits(IfcSchema::IfcThermalResistanceMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_THERMALRESISTANCEUNIT);
	setUnits(IfcSchema::IfcThermalTransmittanceMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_THERMALTRANSMITTANCEUNIT);
	setUnits(IfcSchema::IfcTorqueMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_TORQUEUNIT);
	setUnits(IfcSchema::IfcVaporPermeabilityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_VAPORPERMEABILITYUNIT);
	setUnits(IfcSchema::IfcVolumetricFlowRateMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_VOLUMETRICFLOWRATEUNIT);
	setUnits(IfcSchema::IfcWarpingConstantMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_WARPINGCONSTANTUNIT);
	setUnits(IfcSchema::IfcWarpingMomentMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_WARPINGMOMENTUNIT);

#ifdef SCHEMA_HAS_IfcThermalConductivityMeasure
	setUnits(IfcSchema::IfcThermalConductivityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_THERMALCONDUCTANCEUNIT);
#endif
#ifdef SCHEMA_HAS_IfcNonNegativeLengthMeasure
	setUnits(IfcSchema::IfcNonNegativeLengthMeasure::Class(), IfcSchema::IfcUnitEnum::Value::IfcUnit_LENGTHUNIT);
#endif
#ifdef SCHEMA_HAS_IfcAreaDensityMeasure
	setUnits(IfcSchema::IfcAreaDensityMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_AREADENSITYUNIT);
#endif
#ifdef SCHEMA_HAS_IfcSoundPowerLevelMeasure
	setUnits(IfcSchema::IfcSoundPowerLevelMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_SOUNDPOWERLEVELUNIT);
#endif
#ifdef SCHEMA_HAS_IfcSoundPressureLevelMeasure
	setUnits(IfcSchema::IfcSoundPressureLevelMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_SOUNDPRESSURELEVELUNIT);
#endif
#ifdef SCHEMA_HAS_IfcTemperatureRateOfChangeMeasure
	setUnits(IfcSchema::IfcTemperatureRateOfChangeMeasure::Class(), IfcSchema::IfcDerivedUnitEnum::Value::IfcDerivedUnit_TEMPERATURERATEOFCHANGEUNIT);
#endif
}

void IfcSerialiser::updateUnits()
{
#ifdef SCHEMA_HAS_IfcContext
	auto projects = file->instances_by_type<IfcSchema::IfcContext>();
#else
	auto projects = file->instances_by_type<IfcSchema::IfcProject>();
#endif
	if (projects->size() > 1) {
		repoWarning << "Ifc file has multiple projects or contexts. Units may not be correct for some elements.";
	}

	if (projects->size() > 0) {
		auto project = *projects->begin();
		if (auto assignment = project->UnitsInContext()) {
			auto units = assignment->Units();
			for (auto u : *units)
			{
				if (auto n = u->as<IfcSchema::IfcNamedUnit>()) {
					setUnitsLabel(n->UnitType(), n);
				}
				else if (auto n = u->as<IfcSchema::IfcDerivedUnit>()) {
					setUnitsLabel(n->UnitType(), n);
				}
				else if (auto n = u->as<IfcSchema::IfcMonetaryUnit>()) {
					setUnitsLabel(RepoDerivedUnits::MONETARY_VALUE, n);
				}
			}
		}
	}
}

/*
* The custom filter object is used with the geometry iterator to filter out
* any products we do not want.
*
* This implementation ignores Openings.
*
* IfcSpaces are imported, but appear under their own groups, and so are handled
* further in.
*/
struct IfcSerialiser::filter
{
	bool operator()(IfcUtil::IfcBaseEntity* prod) const {
		if (prod->as<IfcSchema::IfcOpeningElement>()) {
			return false;
		}
		return true;
	}
};

void IfcSerialiser::import(repo::manipulator::modelutility::RepoSceneBuilder* builder)
{
	this->builder = builder;
	createRootNode();

	repoInfo << "Computing units...";

	updateUnits();

	repoInfo << "Computing bounds...";

	updateBounds();

	filter f;
	IfcGeom::Iterator contextIterator("opencascade", settings, file.get(), { boost::ref(f) }, numThreads);
	int previousProgress = 0;
	if (contextIterator.initialize())
	{
		repoInfo << "Processing geometry with " << numThreads << " threads...";

		do
		{
			IfcGeom::Element* element = contextIterator.get();
			import(static_cast<const IfcGeom::TriangulationElement*>(element));

			auto progress = contextIterator.progress();
			if (progress != previousProgress) {
				previousProgress = progress;
				repoInfo << progress << "%...";
			}

		} while (contextIterator.next());

		repoInfo << "100%";
	}
}