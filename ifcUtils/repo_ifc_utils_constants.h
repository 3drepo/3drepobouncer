/**
*  Copyright (C) 2021 3D Repo Ltd
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

#pragma once
#include <string>
#include <set>
#include <unordered_map>

const static std::string IFC_ARGUMENT_GLOBAL_ID = "GlobalId";
const static std::string IFC_ARGUMENT_NAME = "Name";

const static std::string REPO_LABEL_IFC_TYPE = "IFC Type";
const static std::string REPO_LABEL_IFC_GUID = "IFC GUID";

const static std::string IFC_TYPE_ABSORBED_DOSE_MEASURE = "ifcabsorbeddosemeasure";
const static std::string IFC_TYPE_ACCELERATION_MEASURE = "ifcaccelerationmeasure";
const static std::string IFC_TYPE_AMOUNT_OF_SUBSTANCE_MEASURE = "ifcamountofsubstancemeasure";
const static std::string IFC_TYPE_ANGULAR_VELOCITY_MEASURE = "ifcangularvelocitymeasure";
const static std::string IFC_TYPE_ANNOTATIONS = "ifcannotation";
const static std::string IFC_TYPE_AREA_MEASURE = "ifcareameasure";
const static std::string IFC_TYPE_BOOLEAN = "ifcboolean";
const static std::string IFC_TYPE_BUILDING = "ifcbuilding";
const static std::string IFC_TYPE_BUILDING_STOREY = "ifcbuildingstorey";
const static std::string IFC_TYPE_CLASSIFICATION = "ifcclassification";
const static std::string IFC_TYPE_CLASSIFICATION_NOTATION = "ifcclassificationnotation";
const static std::string IFC_TYPE_CLASSIFICATION_REFERENCE = "ifcclassificationreference";
const static std::string IFC_TYPE_COMPLEX_NUMBER = "ifccomplexnumber";
const static std::string IFC_TYPE_COMPLEX_PROPERTY = "ifccomplexproperty";
const static std::string IFC_TYPE_COMPOUND_PLANE_ANGLE_MEASURE = "ifccompoundplaneanglemeasure";
const static std::string IFC_TYPE_CONTEXT_DEPENDENT_MEASURE = "ifccontextdependentmeasure";
const static std::string IFC_TYPE_CONTEXT_DEPENDENT_UNIT = "ifccontextdependentunit";
const static std::string IFC_TYPE_CONVERSION_BASED_UNIT = "ifcconversionbasedunit";
const static std::string IFC_TYPE_COUNT_MEASURE = "ifccountmeasure";
const static std::string IFC_TYPE_CURVATURE_MEASURE = "ifccurvaturemeasure";
const static std::string IFC_TYPE_DERIVED_UNIT = "ifcderivedunit";
const static std::string IFC_TYPE_DERIVED_UNIT_ELEMENT = "ifcderivedunitelement";
const static std::string IFC_TYPE_DESCRIPTIVE_MEASURE = "ifcdescriptivemeasure";
const static std::string IFC_TYPE_DOSE_EQUIVALENT_MEASURE = "ifcdoseequivalentmeasure";
const static std::string IFC_TYPE_DYNAMIC_VISCOSITY_MEASURE = "ifcdynamicviscositymeasure";
const static std::string IFC_TYPE_ELECTRIC_CAPACITANCE_MEASURE = "ifcelectriccapacitancemeasure";
const static std::string IFC_TYPE_ELECTRIC_CHARGE_MEASURE = "ifcelectricchargemeasure";
const static std::string IFC_TYPE_ELECTRIC_CONDUCTANCE_MEASURE = "ifcelectricconductancemeasure";
const static std::string IFC_TYPE_ELECTRIC_CURRENT_MEASURE = "ifcelectriccurrentmeasure";
const static std::string IFC_TYPE_ELECTRIC_RESISTANCE_MEASURE = "ifcelectricresistancemeasure";
const static std::string IFC_TYPE_ELECTRIC_VOLTAGE_MEASURE = "ifcelectricvoltagemeasure";
const static std::string IFC_TYPE_ELEMENT_QUANTITY = "ifcelementquantity";
const static std::string IFC_TYPE_ENERGY_MEASURE = "ifcenergymeasure";
const static std::string IFC_TYPE_FORCE_MEASURE = "ifcforcemeasure";
const static std::string IFC_TYPE_FREQUENCY_MEASURE = "ifcfrequencymeasure";
const static std::string IFC_TYPE_HEATING_VALUE_MEASURE = "ifcheatingvaluemeasure";
const static std::string IFC_TYPE_HEAT_FLUX_DENSITY_MEASURE = "ifcheatfluxdensitymeasure";
const static std::string IFC_TYPE_IDENTIFIER = "ifcidentifier";
const static std::string IFC_TYPE_ILLUMINANCE_MEASURE = "ifcilluminancemeasure";
const static std::string IFC_TYPE_INDUCTANCE_MEASURE = "ifcinductancemeasure";
const static std::string IFC_TYPE_INTEGER = "ifcinteger";
const static std::string IFC_TYPE_INTEGER_COUNT_RATE_MEASURE = "ifcintegercountratemeasure";
const static std::string IFC_TYPE_ION_CONCENTRATION_MEASURE = "ifcionconcentrationmeasure";
const static std::string IFC_TYPE_ISOTHERMAL_MOISTURE_CAPACITY_MEASURE = "ifcisothermalmoisturecapacitymeasure";
const static std::string IFC_TYPE_KINEMATIC_VISCOSITY_MEASURE = "ifckinematicviscositymeasure";
const static std::string IFC_TYPE_LABEL = "ifclabel";
const static std::string IFC_TYPE_LENGTH_MEASURE = "ifclengthmeasure";
const static std::string IFC_TYPE_LINEAR_FORCE_MEASURE = "ifclinearforcemeasure";
const static std::string IFC_TYPE_LINEAR_MOMENT_MEASURE = "ifclinearmomentmeasure";
const static std::string IFC_TYPE_LINEAR_STIFFNESS_MEASURE = "ifclinearstiffnessmeasure";
const static std::string IFC_TYPE_LINEAR_VELOCITY_MEASURE = "ifclinearvelocitymeasure";
const static std::string IFC_TYPE_LOGICAL = "ifclogical";
const static std::string IFC_TYPE_LUMINOUS_FLUX_MEASURE = "ifcluminousfluxmeasure";
const static std::string IFC_TYPE_LUMINOUS_INTENSITY_DISTRIBUTION_MEASURE = "ifcluminousintensitydistributionmeasure";
const static std::string IFC_TYPE_LUMINOUS_INTENSITY_MEASURE = "ifcluminousintensitymeasure";
const static std::string IFC_TYPE_MAGNETIC_FLUX_DENSITY_MEASURE = "ifcmagneticfluxdensitymeasure";
const static std::string IFC_TYPE_MAGNETIC_FLUX_MEASURE = "ifcmagneticfluxmeasure";
const static std::string IFC_TYPE_MASS_DENSITY_MEASURE = "ifcmassdensitymeasure";
const static std::string IFC_TYPE_MASS_FLOW_RATE_MEASURE = "ifcmassflowratemeasure";
const static std::string IFC_TYPE_MASS_MEASURE = "ifcmassmeasure";
const static std::string IFC_TYPE_MASS_PER_LENGTH_MEASURE = "ifcmassperlengthmeasure";
const static std::string IFC_TYPE_MODULUS_OF_ELASTICITY_MEASURE = "ifcmodulusofelasticitymeasure";
const static std::string IFC_TYPE_MODULUS_OF_LINEAR_SUBGRADE_REACTION_MEASURE = "ifcmodulusoflinearsubgradereactionmeasure";
const static std::string IFC_TYPE_MODULUS_OF_ROTATIONAL_SUBGRADE_REACTION_MEASURE = "ifcmodulusofrotationalsubgradereactionmeasure";
const static std::string IFC_TYPE_MODULUS_OF_SUBGRADE_REACTION_MEASURE = "ifcmodulusofsubgradereactionmeasure";
const static std::string IFC_TYPE_MOISTURE_DIFFUSIVITY_MEASURE = "ifcmoisturediffusivitymeasure";
const static std::string IFC_TYPE_MOLECULAR_WEIGHT_MEASURE = "ifcmolecularweightmeasure";
const static std::string IFC_TYPE_MOMENT_OF_INERTIA_MEASURE = "ifcmomentofinertiameasure";
const static std::string IFC_TYPE_MONETARY_MEASURE = "ifcmonetarymeasure";
const static std::string IFC_TYPE_MONETARY_UNIT = "ifcmonetaryunit";
const static std::string IFC_TYPE_NORMALISED_RATIO_MEASURE = "ifcnormalisedratiomeasure";
const static std::string IFC_TYPE_NUMERIC_MEASURE = "ifcnumericmeasure";
const static std::string IFC_TYPE_OPENING_ELEMENT = "ifcopeningelement";
const static std::string IFC_TYPE_PARAMETER_VALUE = "ifcparametervalue";
const static std::string IFC_TYPE_PH_MEASURE = "ifcphmeasure";
const static std::string IFC_TYPE_PLANAR_FORCE_MEASURE = "ifcplanarforcemeasure";
const static std::string IFC_TYPE_PLANE_ANGLE_MEASURE = "ifcplaneanglemeasure";
const static std::string IFC_TYPE_POSITIVE_LENGTH_MEASURE = "ifcpositivelengthmeasure";
const static std::string IFC_TYPE_POSITIVE_PLANE_ANGLE_MEASURE = "ifcpositiveplaneanglemeasure";
const static std::string IFC_TYPE_POSITIVE_RATIO_MEASURE = "ifcpositiveratiomeasure";
const static std::string IFC_TYPE_POWER_MEASURE = "ifcpowermeasure";
const static std::string IFC_TYPE_PRESSURE_MEASURE = "ifcpressuremeasure";
const static std::string IFC_TYPE_PROJECT = "ifcproject";
const static std::string IFC_TYPE_PROPERTY_BOUNDED_VALUE = "ifcpropertyboundedvalue";
const static std::string IFC_TYPE_PROPERTY_SET = "ifcpropertyset";
const static std::string IFC_TYPE_PROPERTY_SINGLE_VALUE = "ifcpropertysinglevalue";
const static std::string IFC_TYPE_QUANTITY_AREA = "ifcquantityarea";
const static std::string IFC_TYPE_QUANTITY_COUNT = "ifcquantitycount";
const static std::string IFC_TYPE_QUANTITY_LENGTH = "ifcquantitylength";
const static std::string IFC_TYPE_QUANTITY_TIME = "ifcquantitytime";
const static std::string IFC_TYPE_QUANTITY_VOLUME = "ifcquantityvolume";
const static std::string IFC_TYPE_QUANTITY_WEIGHT = "ifcquantityweight";
const static std::string IFC_TYPE_RADIO_ACTIVITY_MEASURE = "ifcradioactivitymeasure";
const static std::string IFC_TYPE_RATIO_MEASURE = "ifcratiomeasure";
const static std::string IFC_TYPE_REAL = "ifcreal";
const static std::string IFC_TYPE_REL_AGGREGATES = "ifcrelaggregates";
const static std::string IFC_TYPE_REL_ASSIGNS_TO_GROUP = "ifcrelassignstogroup";
const static std::string IFC_TYPE_REL_ASSOCIATES_CLASSIFICATION = "ifcrelassociatesclassification";
const static std::string IFC_TYPE_REL_CONNECTS_PATH_ELEMENTS = "ifcrelconnectspathelements";
const static std::string IFC_TYPE_REL_CONTAINED_IN_SPATIAL_STRUCTURE = "ifcrelcontainedinspatialstructure";
const static std::string IFC_TYPE_REL_DEFINES_BY_PROPERTIES = "ifcreldefinesbyproperties";
const static std::string IFC_TYPE_REL_DEFINES_BY_TYPE = "ifcreldefinesbytype";
const static std::string IFC_TYPE_REL_NESTS = "ifcrelnests";
const static std::string IFC_TYPE_REL_SPACE_BOUNDARY = "ifcrelspaceboundary";
const static std::string IFC_TYPE_ROTATIONAL_FREQUENCY_MEASURE = "ifcrotationalfrequencymeasure";
const static std::string IFC_TYPE_ROTATIONAL_MASS_MEASURE = "ifcrotationalmassmeasure";
const static std::string IFC_TYPE_ROTATIONAL_STIFFNESS_MEASURE = "ifcrotationalstiffnessmeasure";
const static std::string IFC_TYPE_SECTIONAL_AREA_INTEGRAL_MEASURE = "ifcsectionalareaintegralmeasure";
const static std::string IFC_TYPE_SECTION_MODULUS_MEASURE = "ifcsectionmodulusmeasure";
const static std::string IFC_TYPE_SHEAR_MODULUS_MEASURE = "ifcshearmodulusmeasure";
const static std::string IFC_TYPE_SI_UNIT = "ifcsiunit";
const static std::string IFC_TYPE_SOLID_ANGLE_MEASURE = "ifcsolidanglemeasure";
const static std::string IFC_TYPE_SOUND_POWER_MEASURE = "ifcsoundpowermeasure";
const static std::string IFC_TYPE_SOUND_PRESSURE_MEASURE = "ifcsoundpressuremeasure";
const static std::string IFC_TYPE_SPACE = "ifcspace";
const static std::string IFC_TYPE_SPECIFIC_HEAT_CAPACITY_MEASURE = "ifcspecificheatcapacitymeasure";
const static std::string IFC_TYPE_TEMPERATURE_GRADIENT_MEASURE = "ifctemperaturegradientmeasure";
const static std::string IFC_TYPE_TEXT = "ifctext";
const static std::string IFC_TYPE_THERMAL_ADMITTANCE_MEASURE = "ifcthermaladmittancemeasure";
const static std::string IFC_TYPE_THERMAL_CONDUCTIVITY_MEASURE = "ifcthermalconductivitymeasure";
const static std::string IFC_TYPE_THERMAL_EXPANSION_COEFFICIENT_MEASURE = "ifcthermalexpansioncoefficientmeasure";
const static std::string IFC_TYPE_THERMAL_RESISTANCE_MEASURE = "ifcthermalresistancemeasure";
const static std::string IFC_TYPE_THERMAL_TRANSMITTANCE_MEASURE = "ifcthermaltransmittancemeasure";
const static std::string IFC_TYPE_THERMODYNAMIC_TEMPERATURE_MEASURE = "ifcthermodynamictemperaturemeasure";
const static std::string IFC_TYPE_TIME_MEASURE = "ifctimemeasure";
const static std::string IFC_TYPE_TIME_STAMP = "ifctimestamp";
const static std::string IFC_TYPE_TORQUE_MEASURE = "ifctorquemeasure";
const static std::string IFC_TYPE_VAPOR_PERMEABILITY_MEASURE = "ifcvaporpermeabilitymeasure";
const static std::string IFC_TYPE_VOLUMETRIC_FLOW_RATE_MEASURE = "ifcvolumetricflowratemeasure";
const static std::string IFC_TYPE_VOLUME_MEASURE = "ifcvolumemeasure";
const static std::string IFC_TYPE_WARPING_CONSTANT_MEASURE = "ifcwarpingconstantmeasure";
const static std::string IFC_TYPE_WARPING_MOMENT_MEASURE = "ifcwarpingmomentmeasure";
const static std::set<std::string> typesToIgnore = { IFC_TYPE_REL_ASSIGNS_TO_GROUP, IFC_TYPE_REL_SPACE_BOUNDARY, IFC_TYPE_REL_CONNECTS_PATH_ELEMENTS, IFC_TYPE_ANNOTATIONS, IFC_TYPE_COMPLEX_PROPERTY };

const static std::string IFC_TYPE_IFCREL_PREFIX = "ifcrel";
const static std::string IFC_TYPE_SPACE_LABEL = "(IFC Space)";

const static std::string MONETARY_UNIT = "MONETARYUNIT";

const static std::string LOCATION_LABEL = "Location";
const static std::string PROJECT_LABEL = "Project";
const static std::string BUILDING_LABEL = "Building";
const static std::string STOREY_LABEL = "Storey";
