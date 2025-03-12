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

#include "repo_ifc_utils.h"
#include <repo/error_codes.h>
#include <repo/lib/repo_exception.h>

#include <ifcparse/IfcFile.h>

using namespace ifcUtils;

#define IfcSchema Ifc2x3
#include "repo_ifc_serialiser.h"
#define IfcSchema Ifc4
#include "repo_ifc_serialiser.h"
#define IfcSchema Ifc4x1
#include "repo_ifc_serialiser.h"
#define IfcSchema Ifc4x2
#include "repo_ifc_serialiser.h"
#define IfcSchema Ifc4x3_add2
#include "repo_ifc_serialiser.h"

std::unique_ptr<AbstractIfcSerialiser> IfcUtils::CreateSerialiser(std::string filePath)
{
	auto file = std::make_unique<IfcParse::IfcFile>(filePath);
	if (!file->good()) {
		throw repo::lib::RepoImportException(REPOERR_MODEL_FILE_READ);
	}

	// The Ifc Schema (https://technical.buildingsmart.org/standards/ifc/ifc-schema-specifications/)
	// introduces breaking changes in minor revisions, so each minor revision needs
	// its own Schema. Addendums are upwards compatiable, while Corrigendums do not
	// change the schema.

	auto schema = file->schema()->name();
	if (schema == "IFC2X3")
	{
		return std::make_unique<Ifc2x3Serialiser>(std::move(file));
	}
	else if (schema == "IFC4")
	{
		return std::make_unique<Ifc4Serialiser>(std::move(file));
	}
	else if (schema == "IFC4x1")
	{
		return std::make_unique<Ifc4x1Serialiser>(std::move(file));
	}
	else if (schema == "IFX4x2")
	{
		return std::make_unique<Ifc4x2Serialiser>(std::move(file));
	}
	else if (schema == "IFC4x3" ||
		schema == "IFC4x3_ADD1" ||
		schema == "IFC4X3_ADD2" ||
		schema == "IFC4X3_TC1")
	{
		return std::make_unique<Ifc4x3_add2Serialiser>(std::move(file));
	}
	else
	{
		throw repo::lib::RepoImporterUnavailable(std::string("Unsupported Ifc version: ") + schema, REPOERR_FILE_IFC_UNSUPPORTED_SCHEMA);
	}
}