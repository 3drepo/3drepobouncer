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

#include <OdaCommon.h>
#include <OdString.h>

#include <toString.h>
#include <DgLevelTableRecord.h>

#include <DgAttributeLinkage.h>
#include <FlatMemStream.h>
#include <OdPlatformStreamer.h>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "helper_functions.h"
#include "data_processor_dwg.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

VectoriseDeviceDgn* DataProcessorDwg::device()
{
	return static_cast<VectoriseDeviceDgn*>(OdGsBaseVectorizeView::device());
}

/*
std::unordered_map<std::string, std::string> DataProcessorDwg::extractXMLLinkages(OdDgElementPtr pElm) {
	std::unordered_map<std::string, std::string> entries;

	entries["Element ID"] = convertToStdString(toString(pElm->elementId().getHandle()));

	OdRxObjectPtrArray arrLinkages;
	pElm->getLinkages(OdDgAttributeLinkage::kXmlLinkage, arrLinkages);

	for (OdUInt32 counter = 0; counter < arrLinkages.size(); counter++)
	{
		OdDgAttributeLinkagePtr linkagePtr = arrLinkages[counter];
		if (linkagePtr->getPrimaryId() == OdDgAttributeLinkage::kXmlLinkage) {
			OdDgXmlLinkagePtr pXmlLinkage = OdDgXmlLinkage::cast(linkagePtr);
			if (!pXmlLinkage.isNull()) {
				boost::property_tree::ptree tree;
				std::stringstream ss;
				ss << convertToStdString(pXmlLinkage->getXmlData());
				boost::property_tree::read_xml(ss, tree);
				printTree(tree, entries);
			}
		}
	}

	return entries;
}

// Returns true when the type of element should be part of a grouped hierarchy
// and so the traversal should continue upwards, and false when the type means
// the traversal should terminate.
bool shouldBeInGroupHierarchy(OdDgElement::ElementTypes type)
{
	switch (type) {
	case OdDgElement::ElementTypes::kTypeUnapplicable:
	case OdDgElement::ElementTypes::kTypeUndefined:
		return false;
	default:
		return true;
	}
}
*/

bool DataProcessorDwg::doDraw(OdUInt32 i, const OdGiDrawable* pDrawable)
{
	OdDgElementPtr pElm = OdDgElement::cast(pDrawable);
	auto currentItem = pElm;
	auto previousItem = pElm;

	/*

	while (currentItem->ownerId() && shouldBeInGroupHierarchy(currentItem->getElementType())) {
		previousItem = currentItem;
		auto ownerId = currentItem->ownerId();
		auto ownerItem = OdDgElement::cast(ownerId.openObject(OdDg::kForRead));
		currentItem = ownerItem;
	}
	*/

	//We want to group meshes together up to 1 below the top.
	/*
	std::string groupID = convertToStdString(toString(previousItem->elementId().getHandle()));

	collector->setMeshGroup(groupID);
	std::unordered_map<std::string, std::string> meta;

	OdString sHandle = pElm->isDBRO() ? toString(pElm->elementId().getHandle()) : toString(OD_T("non-DbResident"));
	collector->setNextMeshName(convertToStdString(sHandle));
	*/

	collector->setMeshGroup("Group");
	collector->setNextMeshName("Mesh");
	collector->setLayer("Layer", "Layer Name");

	/*
	OdGiSubEntityTraitsData traits = effectiveTraits();
	OdDgElementId idLevel = traits.layer();
	std::string layerName;
	if (!idLevel.isNull())
	{
		OdDgLevelTableRecordPtr pLevel = idLevel.openObject(OdDg::kForRead);
		const auto levelID = convertToStdString(toString(idLevel.getHandle()));
		layerName = convertToStdString(pLevel->getName());
		collector->setLayer(levelID, layerName);

		if (!collector->hasMeta(levelID)) {
			collector->setMetadata(levelID, extractXMLLinkages(pLevel));
		}
	}

	if (!collector->hasMeta(groupID)) {
		auto meta = extractXMLLinkages(previousItem);
		if (!layerName.empty()) {
			meta["Layer Name"] = layerName;
		}
		collector->setMetadata(groupID, meta);
	}
	*/

	return OdGsBaseMaterialView::doDraw(i, pDrawable);
}

void DataProcessorDwg::convertTo3DRepoMaterial(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData & materialData,
	MaterialColours& matColors,
	repo_material_t& material,
	bool& missingTexture)
{
	DataProcessor::convertTo3DRepoMaterial(prevCache, materialId, materialData, matColors, material, missingTexture);

	/*
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
	*/
}

void DataProcessorDwg::init(GeometryCollector *const geoCollector)
{
	collector = geoCollector;
	//deviationValue = extModel.maxPoint().distanceTo(extModel.minPoint()) / 1e5; //FIXME: Uncomment when dgn bug is fixed https://jira.opendesign.com/browse/DGN-2274
}

void DataProcessorDwg::setMode(OdGsView::RenderMode mode)
{
	OdGsBaseVectorizeView::m_renderMode = kGouraudShaded;
	m_regenerationType = kOdGiRenderCommand;
	OdGiGeometrySimplifier::m_renderMode = OdGsBaseVectorizeView::m_renderMode;
}

void DataProcessorDwg::endViewVectorization()
{
	collector->stopMeshEntry();
	OdGsBaseMaterialView::endViewVectorization();
}

/*
OdCmEntityColor DataProcessorDwg::fixByACI(const ODCOLORREF *ids, const OdCmEntityColor &color)
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
*/