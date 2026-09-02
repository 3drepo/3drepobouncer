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

#include "dwg_proxy_utils.h"
#include "helper_functions.h"

#include <OdString.h>
#include <toString.h>
#include <repo_log.h>
#include <DbEntity.h>
#include <CmColorBase.h>

using namespace repo::manipulator::modelconvertor::odaHelper;

ProxyInfo DwgProxyUtils::getProxyInfo(OdDbEntityPtr entity)
{
	ProxyInfo info = ProxyInfo();

	if (entity.isNull()) 
		return info;

	info.entity = OdDbProxyEntity::cast(entity);
	if (info.entity.isNull()) 
		return info;

	try
	{
		OdString originalClassName = info.entity->originalClassName();
		if (!originalClassName.isEmpty())
		{
			info.originalClass = convertToStdString(originalClassName);
		}
	}
	catch (OdError& e)
	{
		repoWarning << "Failed to get proxy class: " << convertToStdString(e.description());
		info.originalClass = "Unknown";
	}

	info.graphicsType = info.entity->graphicsMetafileType();

	try { info.graphicsPE = OdDbEntityWithGrDataPE::cast(entity); }
	catch (...) {}

	return info;
}

bool DwgProxyUtils::drawStoredProxyGraphics(OdDbEntityPtr pEntity, const ProxyInfo& info, OdGiWorldDraw* worldDraw)
{
	if (pEntity.isNull() || !info.isProxy()) return false;

	if (!info.hasFullGraphics()) return false;
	if (info.graphicsPE.isNull()) return false;

	try
	{
		return info.graphicsPE->worldDraw(pEntity, worldDraw);
	}
	catch (OdError& e)
	{
		repoWarning << "Stored proxy graphics replay failed: " << convertToStdString(e.description());
	}
	catch (...)
	{
		repoWarning << "Stored proxy graphics replay failed with unknown error";
	}

	return false;
}

void DwgProxyUtils::addProxyMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	DwgProxyUtils::addProxyGeneralMetadata(pEntity, metadata);
	if (info.isProxy() && info.isCivil3DSurfaceClass())
	{
		DwgProxyUtils::addProxyGeometryMetadata(pEntity, info, metadata);
	}
}

void DwgProxyUtils::addProxyGeneralMetadata(OdDbEntityPtr pEntity, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	auto colorToString = [](const OdCmColor& clr) {
		switch (clr.colorMethod())
		{
		case OdCmEntityColor::kByLayer:
			return std::string("ByLayer");
		case OdCmEntityColor::kByBlock:
			return std::string("ByBlock");
		case OdCmEntityColor::kByACI:
		case OdCmEntityColor::kByPen:
		case OdCmEntityColor::kByDgnIndex:
		{
			int aci = clr.colorIndex();
			switch (aci)
			{
			case 0: return std::string("ByBlock");
			case 1: return std::string("Red");
			case 2: return std::string("Yellow");
			case 3: return std::string("Green");
			case 4: return std::string("Cyan");
			case 5: return std::string("Blue");
			case 6: return std::string("Magenta");
			case 7: return std::string("White");
			case 256: return std::string("ByLayer");
			default: return std::string("Color ") + std::to_string(aci);
			}
		}
		case OdCmEntityColor::kByColor:
			return std::string("RGB(") + std::to_string(clr.red()) + "," +
				std::to_string(clr.green()) + "," + std::to_string(clr.blue()) + ")";
		case OdCmEntityColor::kForeground:
			return std::string("Foreground");
		case OdCmEntityColor::kNone:
			return std::string("None");
		default:
			return std::string("RGB(") + std::to_string(clr.red()) + "," +
				std::to_string(clr.green()) + "," + std::to_string(clr.blue()) + ")";
		}
		};

	auto lineWeightToString = [](OdDb::LineWeight lw) {
		switch (lw)
		{
		case OdDb::kLnWtByLayer:
			return std::string("ByLayer");
		case OdDb::kLnWtByBlock:
			return std::string("ByBlock");
		case OdDb::kLnWtByLwDefault:
			return std::string("Default");
		default:
			return std::to_string(static_cast<int>(lw) / 100.0) + " mm";
		}
		};

	metadata["General::Layer"] = convertToStdString(toString(pEntity->layer()));
	metadata["General::True Color"] = colorToString(pEntity->color());
	metadata["General::Linetype"] = convertToStdString(toString(pEntity->linetype()));
	metadata["General::Linetype scale"] = pEntity->linetypeScale();
	metadata["General::Lineweight"] = lineWeightToString(pEntity->lineWeight());
	metadata["General::Visibility"] = pEntity->visibility() == OdDb::kInvisible ? std::string("Invisible") : std::string("Visible");
}

void DwgProxyUtils::addProxyGeometryMetadata(OdDbEntityPtr pEntity, const ProxyInfo& info, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	try
	{
		OdGeExtents3d extents;
		if (pEntity->getGeomExtents(extents) == eOk)
		{
			auto min = extents.minPoint();
			auto max = extents.maxPoint();
			metadata["Geometry::Bounds Min"] = "(" + std::to_string(min.x) + ", " + std::to_string(min.y) + ", " + std::to_string(min.z) + ")";
			metadata["Geometry::Bounds Max"] = "(" + std::to_string(max.x) + ", " + std::to_string(max.y) + ", " + std::to_string(max.z) + ")";
			metadata["Geometry::Minimum Elevation"] = min.z;
			metadata["Geometry::Maximum Elevation"] = max.z;
			metadata["Geometry::Number Of Points"] = static_cast<int64_t>(info.currentSurfacePointKeys.size());
		}
	}
	catch (...) {}
}
