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

#include "repo_units.h"

using namespace repo::lib;

static const std::string strlookUp[] = {
	"m", 
	"mm", 
	"cm", 
	"dm", 
	"ft", 
	"in",
	"unknown"
};

static const std::map<std::string, ModelUnits> lookUp = {
	{"m", ModelUnits::METRES},
	{"mm", ModelUnits::MILLIMETRES},
	{"cm", ModelUnits::CENTIMETRES},
	{"dm", ModelUnits::DECIMETRES},
	{"ft", ModelUnits::FEET},
	{"in", ModelUnits::INCHES},
};

static const double toMetreLookUp[] = {
	1.0, 
	0.001, 
	0.01, 
	0.1,
	0.3048, 
	0.0254,
	1.0
};

static const double fromMetreLookUp[] = {
	1.0,
	1000,
	100,
	10,
	1.0 / 0.3048,
	1.0 / 0.0254,
	1.0
};

std::string units::toUnitsString(const ModelUnits& units) {
	int index = (int)units;
	return strlookUp[index];
}

ModelUnits units::fromString(const std::string& unitsStr) {
	auto it = lookUp.find(unitsStr);
	if (it != lookUp.end()) {
		return it->second;
	}
	return ModelUnits::UNKNOWN;
}

double units::scaleFactorToMetres(const ModelUnits& units) {
	int index = (int)units;
	return toMetreLookUp[index];
}

double units::scaleFactorFromMetres(const ModelUnits& units) {
	int index = (int)units;
	return fromMetreLookUp[index];
}

double units::determineScaleFactor(const ModelUnits& base, const ModelUnits& target) {
	if (base == target || base == ModelUnits::UNKNOWN || target == ModelUnits::UNKNOWN) {
		return 1.0;
	}
	auto baseToM = scaleFactorToMetres(base);
	auto mToTarget = scaleFactorFromMetres(target);
	return baseToM * mToTarget;
}