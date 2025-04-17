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
#include <repo/lib/datastructure/repo_variant_utils.h>
#include <boost/variant/apply_visitor.hpp>
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/lib/repo_exception.h"
#include "repo/error_codes.h"

#include <ctime>

#include <ifcparse/IfcEntityInstanceData.h>

// 26812 is a warning about preferring enum class, but these defs are in IFCOS so
// there is nothing we can do about them.
#pragma warning(suppress: 26812)

using namespace ifcUtils;

/*
* The starting index of the subtype attributes - this is the same for Ifc2x3 and
* 4x, however if it changes in future versions we may have to get it from the
* schema instead.
*/
#define NUM_ROOT_ATTRIBUTES 4
#define NUM_PHYSICALSIMPLEQUANTITY_ATTRIBUTES 3

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
static void stringify(std::stringstream& ss, const T& v)
{
	ss << v;
}

static void stringify(std::stringstream& ss, const repo::lib::RepoVariant& v)
{
	ss << boost::apply_visitor(repo::lib::StringConversionVisitor(), v);
}

template<typename T>
static void stringify(std::stringstream& ss, const std::vector<T>& v)
{
	if (v.size()) {
		ss << "(";
		for (size_t i = 0; i < v.size(); i++) {
			stringify(ss, v[i]);
			if (i < v.size() - 1) {
				ss << ", ";
			}
		}
		ss << ")";
	}
}

template<typename T>
static repo::lib::RepoVariant stringify(std::vector<T> vec)
{
	repo::lib::RepoVariant v;
	std::stringstream ss;
	stringify(ss, vec);
	v = ss.str();
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
			v = std::string("UNKNOWN");
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

static const IfcParse::declaration& getPhysicalQuantityValueType(
	const IfcSchema::IfcPhysicalSimpleQuantity* quantity)
{
	if (quantity->as<IfcSchema::IfcQuantityArea>()) {
		return IfcSchema::IfcAreaMeasure::Class();
	}
	else if (quantity->as<IfcSchema::IfcQuantityCount>()) {
		return IfcSchema::IfcCountMeasure::Class();
	}
	else if (quantity->as<IfcSchema::IfcQuantityLength>()) {
		return IfcSchema::IfcLengthMeasure::Class();
	}
	else if (quantity->as<IfcSchema::IfcQuantityTime>()) {
		return IfcSchema::IfcTimeMeasure::Class();
	}
	else if (quantity->as<IfcSchema::IfcQuantityVolume>()) {
		return IfcSchema::IfcVolumeMeasure::Class();
	}
	else if (quantity->as<IfcSchema::IfcQuantityWeight>()) {
		return IfcSchema::IfcMassMeasure::Class();
	}
	return quantity->Class();
}

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

int IfcSerialiser::removeExponentFromString(std::string& base)
{
	// This method will only work for volume and area (superscript 2 and 3), which
	// covers all the base units we define. If the file declares an
	// IfcContextDependentUnit with another exponent in the name however, this will
	// not be supported.

	// This implementation assumes UTF8 encoding, which is safe because we control
	// the encoding of this file, which is also the one in which the units are
	// defined.

	auto length = base.size();
	if (length > 2) {
		auto data = base.data();
		if (data[length - 2] == (char)0xC2) {
			if (data[length - 1] == (char)0xB2) {
				base.resize(length - 2);
				return 2;
			}
			else if (data[length - 1] == (char)0xB3) {
				base.resize(length - 2);
				return 3;
			}
		}
	}
	return 1;
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

	// Some conversion unit names are identical to the units themselves. We only
	// check below for those we have different symbols for.

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
	if (data == "fahrenheit") return "°F";

	return data;
}

std::string IfcSerialiser::getUnitsLabel(const IfcSchema::IfcUnit* unit)
{
	if (!unit)
	{
		return {};
	}
	else if (auto u = unit->as<IfcSchema::IfcSIUnit>())
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
		std::string multiplier = "";
		for (const auto& e : *elements)
		{
			auto base = getUnitsLabel(e->Unit());
			if (!base.empty())
			{
				auto ex = removeExponentFromString(base);
				ss << multiplier << base << getExponentAsString(e->Exponent() * ex);
			}
			else
			{
				std::stringstream es;
				e->Unit()->data().toString(es);
				repoWarning << "Unrecognised Derived Unit Element " << es.str();
			}
			multiplier = "·";
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
	return {};
}

std::string IfcSerialiser::getUnitsLabel(const unit_t& unit)
{
	auto label = unitLabels.find(unit);
	if (label != unitLabels.end()) {
		return label->second;
	}
	return {};
}

std::string IfcSerialiser::getUnitsLabel(const aggregate_of<IfcSchema::IfcValue>::ptr list)
{
	if (list && list->size()) {
		return getUnitsLabel((*list->begin())->declaration());
	}
	else {
		return {};
	}
}

std::string IfcSerialiser::getUnitsLabel(const boost::optional<aggregate_of<IfcSchema::IfcValue>::ptr> list)
{
	return getUnitsLabel(list.get_value_or(nullptr));
}

void IfcSerialiser::setNamedTypeUnits()
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
	setUnits(IfcSchema::IfcMonetaryMeasure::Class(), RepoDerivedUnits::MONETARY_VALUE);

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

void IfcSerialiser::setUnits(IfcSchema::IfcSIUnit u)
{
	unitLabels[u.UnitType()] = getUnitsLabel(&u);
}

void IfcSerialiser::setUnits(const IfcParse::type_declaration& type, unit_t units)
{
	definedValueTypeUnits[type.index_in_schema()] = units;
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
	setNamedTypeUnits();
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

void IfcSerialiser::setLevelOfDetail(int lod)
{
	std::pair<double, double> params = { 0.001, 0.5 };

	switch (lod) {
	case 1:
		params = { 1000, 1 };
		break;
	case 2:
		params = { 0.5, 0.698132 };
		break;
	case 3:
		params = { 0.1, 0.25 };
		break;
	case 4:
		params = { 0.01, 0.174 };
		break;
	case 5:
		params = { 0.005, 0.05 };
		break;
	case 6:
		params = { 0.0005, 0.0174 };
		break;
	}

	settings.get<ifcopenshell::geometry::settings::MesherLinearDeflection>().value = std::get<0>(params);
	settings.get<ifcopenshell::geometry::settings::MesherAngularDeflection>().value = std::get<1>(params);
}

void IfcSerialiser::setNumThreads(int numThreads)
{
	this->numThreads = numThreads ? numThreads : std::thread::hardware_concurrency();
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

		if (ptr->get_color()) {
			material.diffuse = repoColor(ptr->get_color());
		}

		if (ptr->specular) {
			material.specular = repoColor(ptr->specular);
		}

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
	}

	return nullptr;
}

/*
* Decides based on the parent-object relationship, whether to insert an
* additional branch node in order to group child objects by their Ifc type
* in our tree.
*/
bool IfcSerialiser::shouldGroupByType(const IfcSchema::IfcObjectDefinition* object, const typename IfcSchema::IfcObjectDefinition* parent)
{
	return parent && !parent->as<IfcSchema::IfcElement>() && object->as<IfcSchema::IfcElement>();
}

std::shared_ptr<repo::core::model::TransformationNode> IfcSerialiser::createTransformationNode(
	const IfcSchema::IfcObjectDefinition* object,
	const repo::lib::RepoUUID& parentId)
{
	auto name = object->Name().get_value_or({});
	if (name.empty())
	{
		name = object->declaration().name();
	}

	auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode({}, name, { parentId });

	auto it = metadataUniqueIds.find(object->id());
	if (it != metadataUniqueIds.end())
	{
		builder->addParent(it->second, transform.getSharedID());
	}
	else
	{
		auto metaNode = createMetadataNode(object);
		metaNode->addParent(transform.getSharedID());
		metadataUniqueIds[object->id()] = metaNode->getUniqueID();
		builder->addNode(std::move(metaNode));
	}

	return builder->addNode(transform);;
}

repo::lib::RepoUUID IfcSerialiser::getTransformationNode(const IfcSchema::IfcObjectDefinition* parent, const IfcParse::entity& typeGroup)
{
	auto parentId = getTransformationNode(parent, false);
	auto groupId = parent->file_->getMaxId() + parent->id() + typeGroup.type() + 1;
	auto& nodes = sharedIds[groupId];

	if (nodes.branchSharedId) {
		return nodes.branchSharedId;
	}

	auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode({}, typeGroup.name(), { parentId });
	builder->addNode(transform);

	nodes.branchSharedId = transform.getSharedID();

	return transform.getSharedID();
}

repo::lib::RepoUUID IfcSerialiser::getTransformationNode(const IfcSchema::IfcObjectDefinition* object, bool leafNode)
{
	if (!object)
	{
		return rootNodeId;
	}

	auto& nodes = sharedIds[object->id()];

	// Ifc Objects may have both geometry representations and children, but this is
	// not supported in our tree.

	// As such, we allow each object to create up to two transformation nodes to
	// represent it: a leaf node to hold only its meshes, and a branch node to hold
	// its children. These nodes are are built on-demand. Depending on which is
	// created first this can be a simple addition, but in the most complex involves
	// re-parenting existing leaf nodes to insert a new common parent.

	if (leafNode && nodes.leafNode)
	{
		return nodes.leafNode->getSharedID();
	}
	else if (!leafNode && nodes.branchSharedId)
	{
		return nodes.branchSharedId;
	}
	else if (leafNode && nodes.branchSharedId)
	{
		nodes.leafNode = createTransformationNode(object, nodes.branchSharedId);
		return nodes.leafNode->getSharedID();
	}
	else if (!leafNode && nodes.leafNode)
	{
		nodes.branchSharedId = createTransformationNode(object, nodes.leafNode->getParentIDs()[0])->getSharedID();
		nodes.leafNode->setParents({ nodes.branchSharedId });
		return nodes.branchSharedId;
	}
	else
	{
		auto parent = getParent(object);

		// If the parent is a non-physical entity, group elements by type, as
		// a convenience to the user.

		repo::lib::RepoUUID parentId;
		if (shouldGroupByType(object, parent))
		{
			parentId = getTransformationNode(parent, object->declaration());
		}
		else
		{
			parentId = getTransformationNode(parent, false);
		}

		auto node = createTransformationNode(object, parentId);

		if (leafNode)
		{
			nodes.leafNode = node;
		}
		else
		{
			nodes.branchSharedId = node->getSharedID();
		}

		return node->getSharedID();
	}
}

repo::lib::RepoUUID IfcSerialiser::getParentId(const IfcGeom::Element* element, bool createTransform)
{
	auto object = element->product()->as<IfcSchema::IfcObjectDefinition>();
	if (createTransform)
	{
		return getTransformationNode(object, true);
	}
	else
	{
		auto parent = getParent(object);
		if (shouldGroupByType(object, parent))
		{
			return getTransformationNode(parent, object->declaration());
		}
		else
		{
			return getTransformationNode(parent, false);
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

	bool isIfcSpace = triangulation->product()->as<IfcSchema::IfcSpace>();

	std::string name;
	if (isIfcSpace) {
		name = triangulation->name() + " (IFC Space)";
	}

	auto parentId = getParentId(triangulation, !isIfcSpace);

	std::unique_ptr<repo::core::model::MetadataNode> metaNode;
	if (isIfcSpace) {
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

IfcSerialiser::RepoValue IfcSerialiser::getValue(const IfcSchema::IfcObjectReferenceSelect* object)
{
	// The IfcObjectReferenceSelect adds an object as an assignment. The objects
	// that can be assigned this way are limited:
	// https://standards.buildingsmart.org/IFC/DEV/IFC4_2/FINAL/HTML/schema/ifcpropertyresource/lexical/ifcobjectreferenceselect.htm

	// This method is used when we want an identifier of an object for a key-value
	// pair, as opposed to when we want to recurse into the object to gather its
	// properties directly.

	// Some types we do not format, and are simply ignored. We leave them in the
	// if..else tree for documentation purposes.

	if (auto o = object->as<IfcSchema::IfcPerson>())
	{
#ifdef SCHEMA_IfcPerson_HAS_Identification
		if (o->Identification()) {
			return { o->Identification().value(), {} };
		}
#endif
	}
#ifdef SCHEMA_HAS_IfcMaterialDefinition
	else if (auto o = object->as<IfcSchema::IfcMaterialDefinition>())
	{
	}
#endif
	else if (auto o = object->as<IfcSchema::IfcOrganization>())
	{
#ifdef SCHEMA_IfcOrganization_HAS_Identification
		if (o->Identification()) {
			return { o->Identification().value(), {} };
		}
#endif
	}
	else if (auto o = object->as<IfcSchema::IfcPersonAndOrganization>())
	{
#ifdef SCHEMA_IfcPerson_HAS_Identification
		if (o->ThePerson() && o->TheOrganization()) {
			return {
				o->ThePerson()->Identification().value() + "(" + o->TheOrganization()->Identification().value() + ")",
				{}
			};
		}
#endif
	}
	else if (auto o = object->as<IfcSchema::IfcExternalReference>())
	{
		if (o->Location()) {
			return { o->Location().value(), {} };
		}
	}
	else if (auto o = object->as<IfcSchema::IfcTimeSeries>())
	{
	}
	else if (auto o = object->as<IfcSchema::IfcAddress>())
	{
	}
	else if (auto o = object->as<IfcSchema::IfcAppliedValue>())
	{
		if (auto v = o->AppliedValue()->as<IfcSchema::IfcValue>()) {
			return getValue(v);
		}
		else if (auto v = o->AppliedValue()->as<IfcSchema::IfcMeasureWithUnit>()) {
			auto f = getValue(v->ValueComponent());
			f.units = getUnitsLabel(v->UnitComponent());
			return f;
		}
	}
	else if (auto o = object->as<IfcSchema::IfcTable>())
	{
	}

	return {};
}

std::optional<repo::lib::RepoVariant> IfcSerialiser::getValue(boost::optional<aggregate_of<IfcSchema::IfcValue>::ptr> list)
{
	if (list) {
		return getValue(list.value());
	}
	else {
		return std::nullopt;
	}
}

std::optional<repo::lib::RepoVariant> IfcSerialiser::getValue(aggregate_of<IfcSchema::IfcValue>::ptr list)
{
	std::vector<repo::lib::RepoVariant> values;

	for (auto& v : *list)
	{
		auto f = getValue(v);
		if (f.v) {
			values.push_back(*f.v);
		}
	}

	if (values.size() < 1)
	{
		return std::nullopt;
	}
	else if (values.size() < 2)
	{
		return values[0];
	}
	else
	{
		return stringify(values);
	}
}

IfcSerialiser::RepoValue IfcSerialiser::getValue(const IfcParse::declaration& type, const AttributeValue& value)
{
	// The AttributeValue object contains low-level dynamic type information about
	// the primitive(s) it contains. For example, whether its an integer or floating
	// point, and whether it is a single value or an array. How these should be
	// interpreted however depends on the outer type.

	// For example, an IfcLengthMeasure holds a double. The IfcLengthMeasure type
	// itself indicates that it is a length, and that it uses the LENGTHUNIT
	// units.

	// Most types simply wrap a basic primitive, and the type information in
	// AttributeValue is enough. Some types however require further processing to
	// present them properly, which we check for and apply below...

	if (type.is(IfcSchema::IfcCompoundPlaneAngleMeasure::Class())) {
		auto c = static_cast<std::vector<int>>(value);
		std::stringstream ss;
		ss << c[0] << "° ";
		ss << abs(c[1]) << "' ";
		ss << abs(c[2]) << "\" ";
		ss << abs(c[3]);
		repo::lib::RepoVariant v;
		v = ss.str();
		return { v, {} };
	}

	if (type.is(IfcSchema::IfcComplexNumber::Class()))
	{
		auto c = static_cast<std::vector<double>>(value);
		std::string v = std::to_string(c[0]) + "+" + std::to_string(c[1]) + "i";
		return { v, {} };
	}

	if (type.is(IfcSchema::IfcTimeStamp::Class()))
	{
		// (Other date-time types (IfcDate, IfcTime and IfcDateTime) are string encoded already)
		auto c = (time_t)static_cast<int>(value);
		std::tm timestamp = *std::gmtime(&c);
		return { timestamp, {} };
	}

	return {
		repoVariant(value),
		getUnitsLabel(type)
	};
}

IfcSerialiser::RepoValue IfcSerialiser::getValue(const IfcSchema::IfcValue* value)
{
	// IfcValues are Select types, with the subclass being one of the named types
	// indicating how the primitive should be interpreted. The primitive itself
	// is stored in attribute 0 (see the conversion operator implementations in
	// IFCOS...).

	return getValue(value->declaration(), value->data().get_attribute_value(0));
}

void IfcSerialiser::collectAttributes(const IfcUtil::IfcBaseEntity* object, Metadata& metadata)
{
	// Entities are the most abstract type that can hold attributes. This method
	// adds all those attributes to the metadata object.

	auto start = 0;
	if (object->as<IfcSchema::IfcRoot>()) {

		// IfcRoot is the root class for all kernel types. It has a fixed number
		// of attributes - these are handled explicitly for the top-level object,
		// so are always skipped here.

		// Ifc Resource Schema types can have attributes, but do not derive from
		// root, so should not be skipping anything.

		start += NUM_ROOT_ATTRIBUTES;
	}

	auto type = object->declaration().as_entity();
	for (size_t i = start; i < type->attribute_count(); i++)
	{
		auto attribute = type->attribute_by_index(i);
		auto value = object->data().get_attribute_value(i);

		if (!value.isNull())
		{
			// If the attribute is a reference to another object, then we attempt to
			// collect that object's metadata under a local group. Usually attributes
			// will only be simple, however there are a number of cases, such as with
			// predefined types, where static attributes reference entities that have
			// relevant metadata.

			// If the value is not a reference, the type information is stored with
			// the attribute itself, rather than the value.

			if (value.type() == IfcUtil::ArgumentType::Argument_ENTITY_INSTANCE)
			{
				auto suffixed = metadata.addSuffix(attribute->name());
				collectMetadata((const IfcUtil::IfcBaseClass*)value, suffixed);
			}
			else if (value.type() == IfcUtil::ArgumentType::Argument_AGGREGATE_OF_ENTITY_INSTANCE)
			{
				auto refs = value.operator boost::shared_ptr<aggregate_of_instance>();
				auto i = 0;
				for (auto& ref : *refs) {
					auto suffixed = metadata.addSuffix(attribute->name() + " " + std::to_string(i++));
					collectMetadata((const IfcUtil::IfcBaseClass*)ref, suffixed);
				}
			}
			else
			{
				auto attribute_type = attribute->type_of_attribute();
				RepoValue v;
				if (auto named = attribute_type->as_named_type())
				{
					v = getValue(*named->declared_type(), value);
				}
				else if (auto simple = attribute_type->as_simple_type())
				{
					v.v = repoVariant(value);
				}
				else if (auto aggregation = attribute_type->as_aggregation_type())
				{
					v.v = repoVariant(value);
					if (auto named = aggregation->type_of_element()->as_named_type()) {
						v.units = getUnitsLabel(*named->declared_type());
					}
				}
				metadata.setValue(attribute->name(), v);
			}
		}
	}
}

template<typename T>
void IfcSerialiser::collectMetadata(T begin, T end, Metadata& metadata)
{
	for (auto it = begin; it != end; it++) {
		collectMetadata(*it, metadata);
	}
}

void IfcSerialiser::collectMetadata(const IfcUtil::IfcBaseInterface* object, Metadata& metadata)
{
	if (auto o = object->as<IfcSchema::IfcRelDefinesByProperties>())
	{
#ifdef SCHEMA_HAS_IfcPropertySetDefinitionSelect
		if (auto p = o->RelatingPropertyDefinition()->as<IfcSchema::IfcPropertySetDefinitionSet>()) {
			// ProperySetDefinitionSets are temporarily disabled until this IFCOS bug is resolved: https://github.com/IfcOpenShell/IfcOpenShell/issues/6330
			//auto props = p->operator boost::shared_ptr<aggregate_of<IfcSchema::IfcPropertySetDefinition>>();
			//collectMetadata(props->begin(), props->end(), metadata);
		}else if (auto s = o->RelatingPropertyDefinition()->as<IfcSchema::IfcPropertySetDefinition>()) {
			collectMetadata(s, metadata);
		}
#else
		collectMetadata(o->RelatingPropertyDefinition(), metadata);
#endif
	}
	else if (auto o = object->as<IfcSchema::IfcRelNests>())
	{
		// IfcRelNests is used to build a graph of non-physical entities such as
		// processes and controls. Since we don't typically represent such entities
		// in the tree, they are combined into the metadata of the top-level node.

		auto related = o->RelatedObjects();
		auto noOverwrite = metadata.setOverwrite(false);
		collectMetadata(related->begin(), related->end(), noOverwrite);
	}
	else if (auto o = object->as<IfcSchema::IfcRelDefinesByType>())
	{
		if (auto type = o->RelatingType()) {
			auto noOverwrite = metadata.setOverwrite(false);
			if (auto props = type->HasPropertySets().get_value_or({})) {
				collectMetadata(props->begin(), props->end(), noOverwrite);
			}
			auto related = file->getInverse(type->id(), &(IfcSchema::IfcRelDefinesByProperties::Class()), -1);
			collectMetadata(related->begin(), related->end(), noOverwrite);
		}
	}
#ifdef SCHEMA_HAS_IfcRelDefinesByTemplate
	else if (auto o = object->as<IfcSchema::IfcRelDefinesByTemplate>())
	{
		// Not currently used
	}
#endif
#ifdef SCHEMA_HAS_IfcRelDefinesByObject
	else if (auto o = object->as<IfcSchema::IfcRelDefinesByObject>())
	{
		if (auto relating = o->RelatingObject()) {
			auto props = file->getInverse(relating->id(), &(IfcSchema::IfcRelDefinesByProperties::Class()), -1);
			collectMetadata(props->begin(), props->end(), metadata);
		}
	}
#endif
	else if (auto o = object->as<IfcSchema::IfcRelAssociatesClassification>())
	{
		collectMetadata(o->RelatingClassification(), metadata);
	}
	else if (auto o = object->as<IfcSchema::IfcClassificationReference>())
	{
		if (o->ReferencedSource()) {
			collectMetadata(o->ReferencedSource(), metadata);
		} else {
			auto ctx = metadata.setGroup(o->Name().get_value_or(o->declaration().name()));
			collectAttributes(o, ctx);
		}
	}
	else if (auto o = object->as<IfcSchema::IfcClassification>())
	{
		auto ctx = metadata.setGroup(o->Name());
		collectAttributes(o, ctx);
	}
	else if (auto o = object->as<IfcSchema::IfcPropertySet>())
	{
		auto ctx = metadata.setGroup(o->Name().get_value_or(o->declaration().name()));
		auto props = o->HasProperties();
		collectMetadata(props->begin(), props->end(), ctx);
	}
#ifdef SCHEMA_HAS_IfcPreDefinedPropertySet
	else if (auto o = object->as<IfcSchema::IfcPreDefinedPropertySet>())
	{
		auto ctx = metadata.setGroup(o->Name().get_value_or(o->declaration().name()));
		collectAttributes(o, ctx);
	}
#endif
#ifdef SCHEMA_HAS_IfcPreDefinedProperties
	else if (auto o = object->as<IfcSchema::IfcPreDefinedProperties>())
	{
		collectAttributes(o, metadata);
	}
#endif
	else if (auto o = object->as<IfcSchema::IfcProfileDef>())
	{
		collectAttributes(o, metadata);
	}
	else if (auto o = object->as<IfcSchema::IfcComplexProperty>())
	{
		// Our metadata groups are at most one deep, so complex properties are
		// unrolled and an identifier suffixed to the key.

		if (auto props = o->HasProperties()) {
			Metadata ctx = metadata.addSuffix(o->UsageName());
			collectMetadata(props->begin(), props->end(), ctx);
		}
	}
	else if (auto o = object->as<IfcSchema::IfcPropertyBoundedValue>())
	{
		auto units = getUnitsLabel(o->Unit());
		std::string upperBound, lowerBound;
		if (o->UpperBoundValue())
		{
			auto variant = getValue(o->UpperBoundValue());
			upperBound = boost::apply_visitor(repo::lib::StringConversionVisitor(), *variant.v);
			if (units.empty()) units = variant.units;
		}
		if (o->LowerBoundValue())
		{
			auto variant = getValue(o->LowerBoundValue());
			lowerBound = boost::apply_visitor(repo::lib::StringConversionVisitor(), *variant.v);
			if (units.empty()) units = variant.units;
		}
		metadata.setValue(o->Name(), { "[" + lowerBound + ", " + upperBound + "]", units });
	}
	else if (auto o = object->as<IfcSchema::IfcPropertyEnumeratedValue>())
	{
		auto units = getUnitsLabel(o->EnumerationValues());
		if (o->EnumerationReference() && o->EnumerationReference()->Unit()) {
			units = getUnitsLabel(o->EnumerationReference()->Unit());
		}
		metadata.setValue(o->Name(), { getValue(o->EnumerationValues()), units });
	}
	else if (auto o = object->as<IfcSchema::IfcPropertyListValue>())
	{
		auto units = getUnitsLabel(o->ListValues());
		if (o->Unit()) {
			units = getUnitsLabel(o->Unit());
		}
		metadata.setValue(o->Name(), { getValue(o->ListValues()), units });
	}
	else if (auto o = object->as<IfcSchema::IfcPropertyReferenceValue>())
	{
		metadata.setValue(o->Name(), getValue(o->PropertyReference()));
	}
	else if (auto o = object->as<IfcSchema::IfcPropertySingleValue>())
	{
		if (o->NominalValue()) {
			auto f = getValue(o->NominalValue());
			if (o->Unit()) {
				f.units = getUnitsLabel(o->Unit());
			}
			metadata.setValue(o->Name(), f);
		}
	}
	else if (auto table = object->as<IfcSchema::IfcPropertyTableValue>())
	{
		// This is ignored for now as it is not clear how we would format it,
		// and none of the other tools display it.
	}
#ifdef SCHEMA_HAS_IfcElementQuantity
	else if (auto o = object->as<IfcSchema::IfcElementQuantity>())
	{
		auto ctx = metadata.setGroup(o->Name().get_value_or(o->declaration().name()));
		auto method = o->MethodOfMeasurement().get_value_or({});
		if (!method.empty()) {
			ctx("MethodOfMeasurement") = method;
		}
		auto quantities = o->Quantities();
		collectMetadata(quantities->begin(), quantities->end(), ctx);
	}
#endif
	else if (auto o = object->as<IfcSchema::IfcPhysicalSimpleQuantity>())
	{
		// All IfcPhysicalSimpleQuantity subtypes have their values at the same
		// attribute, so we don't need to check each type individually.

		// We do however need to infer the type of the value, as they will be
		// specific Measure types, and this information is not stored in
		// AttributeValue or IfcPhysicalSimpleQuantity programmatically.

		auto v = getValue(getPhysicalQuantityValueType(o), o->data().get_attribute_value(NUM_PHYSICALSIMPLEQUANTITY_ATTRIBUTES));
		if (o->Unit()) {
			v.units = getUnitsLabel(o->Unit());
		}
		metadata.setValue(o->Name(), v);
	}
	else if (auto o = object->as<IfcSchema::IfcPhysicalComplexQuantity>())
	{
		auto quantities = o->HasQuantities();
		auto suffix = metadata.addSuffix(o->Usage().get_value_or(o->Name()));
		collectMetadata(quantities->begin(), quantities->end(), suffix);
	}
}

/*
For some types, build explicit metadata entries
*/
void IfcSerialiser::collectAdditionalAttributes(const IfcSchema::IfcObjectDefinition* object, Metadata& metadata)
{
	auto location = metadata.setGroup(LOCATION_LABEL);

	if (auto project = dynamic_cast<const IfcSchema::IfcProject*>(object)) {
		location(PROJECT_LABEL) = project->Name().get_value_or("(" + object->declaration().name() + ")");
	}

	if (auto building = dynamic_cast<const IfcSchema::IfcBuilding*>(object)) {
		location(BUILDING_LABEL) = building->Name().get_value_or("(" + object->declaration().name() + ")");
	}

	if (auto storey = dynamic_cast<const IfcSchema::IfcBuildingStorey*>(object)) {
		location(STOREY_LABEL) = storey->Name().get_value_or("(" + object->declaration().name() + ")");
	}
}

std::unique_ptr<repo::core::model::MetadataNode> IfcSerialiser::createMetadataNode(const IfcSchema::IfcObjectDefinition* object)
{
	std::unordered_map<std::string, repo::lib::RepoVariant> map;
	Metadata metadata(map);

	// To build a metadata node, the serialiser first collects the IfcRoot
	// attributes for the starting object (the one that will be the leaf
	// node in the tree). It then collects type-specific attributes, before
	// beginning a traversal of the Ifc Relationship objects.

	auto& type = object->declaration();
	auto& instance = object->data();

	metadata(REPO_LABEL_IFC_TYPE) = type.name();
	metadata(REPO_LABEL_IFC_NAME) = object->Name().get_value_or("(" + type.name() + ")");
	metadata(REPO_LABEL_IFC_GUID) = object->GlobalId();
	if (object->Description()) {
		metadata(REPO_LABEL_IFC_DESCRIPTION) = object->Description().value();
	}

	collectAttributes(object, metadata);

	collectAdditionalAttributes(object, metadata);

	// Ifc objects are given properties by associating with containers, such as
	// Property Sets, using instances of IfcRelationship. IfcRelationship
	// are also used to build the tree; the subtype of IfcRelationship determines
	// the meaning of the related objects. Below we consider a number of
	// relationship instances that associate metadata, and traverse them to extract
	// that metadata into a single node.

	auto d = file->getInverse(object->id(), &(IfcSchema::IfcRelDefines::Class()), -1);
	collectMetadata(d->begin(), d->end(), metadata);

	auto n = file->getInverse(object->id(), &(IfcSchema::IfcRelNests::Class()), -1);
	collectMetadata(n->begin(), n->end(), metadata);

	auto a = file->getInverse(object->id(), &(IfcSchema::IfcRelAssociates::Class()), -1);
	collectMetadata(a->begin(), a->end(), metadata);

	auto metadataNode = std::make_unique<repo::core::model::MetadataNode>(repo::core::model::RepoBSONFactory::makeMetaDataNode(map, {}, {}));
	return metadataNode;
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
* This implementation ignores Openings, so the openings will be subtracted from
* the host geometry, but will not exist as entities in their own right in the
* tree.
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

		repoInfo << "Done";
	}

	sharedIds.clear(); // This call releases any leaf nodes, now we are sure they won't change.
}