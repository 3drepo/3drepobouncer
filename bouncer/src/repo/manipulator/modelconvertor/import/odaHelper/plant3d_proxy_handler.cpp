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

#include "plant3d_proxy_handler.h"

#include "helper_functions.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

namespace {
	// Known Plant3D extension dictionary entries, and the substrings used to spot
	// a property name resbuf within them. Const at namespace scope, so these have
	// internal linkage and are built once rather than per entity.
	const std::vector<std::string> kPlant3DDicts = {
		"ACAD_XREC_ROUNDTRIP",
		"AcPpDb3dPart",
		"AcPpDb3dSpecPart",
		"PartSizeProperties"
	};
	const std::vector<std::string> kPlant3DTriggers = { "Tag", "Service", "Size", "NominalDiameter", "Spec", "PartFamily" };
}

bool Plant3DProxyHandler::matches(const std::string& originalClass) const
{
	return originalClass.find("AcPp") != std::string::npos ||
		originalClass.find("Plant") != std::string::npos;
}

void Plant3DProxyHandler::addDictionaryMetadata(
	OdDbDictionaryPtr dict,
	std::unordered_map<std::string, repo::lib::RepoVariant>& metadata) const
{
	// Plant3D's original parser never handled kDxfInt32 resbufs, unlike Civil3D's.
	extractProxyDictionaryProperties(dict, kPlant3DDicts, "Plant3D", kPlant3DTriggers, false, metadata);
}
