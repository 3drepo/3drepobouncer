/**
*  Copyright (C) 2018 3D Repo Ltd
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

#include <OdaCommon.h>
#include <OdString.h>

#include <toString.h>
#include <DgLevelTableRecord.h>

#include "helper_functions.h"
#include "geometry_dumper_dgn.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

VectoriseDeviceDgn* GeometryDumperDgn::device()
{
	return static_cast<VectoriseDeviceDgn*>(OdGsBaseVectorizeView::device());
}

bool GeometryDumperDgn::doDraw(OdUInt32 i, const OdGiDrawable* pDrawable)
{
	OdDgElementPtr pElm = OdDgElement::cast(pDrawable);
	auto currentItem = pElm;
	auto previousItem = pElm;
	while (currentItem->ownerId()) {
		previousItem = currentItem;
		auto ownerId = currentItem->ownerId();
		auto ownerItem = OdDgElement::cast(ownerId.openObject(OdDg::kForRead));
		currentItem = ownerItem;
	}

	//We want to group meshes together up to 1 below the top.
	OdString groupID = toString(previousItem->elementId().getHandle());
	collector->setMeshGroup(convertToStdString(groupID));

	OdString sHandle = pElm->isDBRO() ? toString(pElm->elementId().getHandle()) : toString(OD_T("non-DbResident"));
	collector->setNextMeshName(convertToStdString(sHandle));

	OdGiSubEntityTraitsData traits = effectiveTraits();
	OdDgElementId idLevel = traits.layer();
	if (!idLevel.isNull())
	{
		OdDgLevelTableRecordPtr pLevel = idLevel.openObject(OdDg::kForRead);
		collector->setLayer(convertToStdString(pLevel->getName()));
	}
	return OdGsBaseMaterialView::doDraw(i, pDrawable);
}

void GeometryDumperDgn::convertTo3DRepoMaterial(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData,
	MaterialColors& matColors,
	repo_material_t& material,
	bool& missingTexture)
{
	DataProcessor::convertTo3DRepoMaterial(prevCache, materialId, materialData, matColors, material, missingTexture);

	OdCmEntityColor color = fixByACI(this->device()->getPalette(), effectiveTraits().trueColor());
	// diffuse
	if (matColors.colorDiffuseOverride)
		material.diffuse = { ODGETRED(matColors.colorDiffuse) / 255.0f, ODGETGREEN(matColors.colorDiffuse) / 255.0f, ODGETBLUE(matColors.colorDiffuse) / 255.0f, 1.0f };
	else
		material.diffuse = { color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f, 1.0f };
	// specular
	if (matColors.colorSpecularOverride)
		material.specular = { ODGETRED(matColors.colorSpecular) / 255.0f, ODGETGREEN(matColors.colorSpecular) / 255.0f, ODGETBLUE(matColors.colorSpecular) / 255.0f, 1.0f };
	else
		material.specular = { color.red() / 255.0f, color.green() / 255.0f, color.blue() / 255.0f, 1.0f };

	material.shininessStrength = 1 - material.shininessStrength;
}

void GeometryDumperDgn::init(GeometryCollector *const geoCollector)
{
	collector = geoCollector;
}

void GeometryDumperDgn::setMode(OdGsView::RenderMode mode)
{
	OdGsBaseVectorizeView::m_renderMode = kGouraudShaded;
	m_regenerationType = kOdGiRenderCommand;
	OdGiGeometrySimplifier::m_renderMode = OdGsBaseVectorizeView::m_renderMode;
}

void GeometryDumperDgn::endViewVectorization()
{
	collector->stopMeshEntry();
	OdGsBaseMaterialView::endViewVectorization();
}

OdCmEntityColor GeometryDumperDgn::fixByACI(const ODCOLORREF *ids, const OdCmEntityColor &color)
{
	if (color.isByACI() || color.isByDgnIndex())
	{
		return OdCmEntityColor(ODGETRED(ids[color.colorIndex()]), ODGETGREEN(ids[color.colorIndex()]), ODGETBLUE(ids[color.colorIndex()]));
	}
	else if (!color.isByColor())
	{
		return OdCmEntityColor(0, 0, 0);
	}
	return color;
}