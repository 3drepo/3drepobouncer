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
#include <DbObject.h>
#include <DbEntity.h>
#include <DbLayout.h>
#include <DbLayerTableRecord.h>
#include <DbStubPtrArray.h>
#include <DbDictionary.h>
#include <DbBlockReference.h>
#include <DbLine.h>
#include <OdString.h>
#include <DgCmColor.h>
#include <DgLevelTableRecord.h>
#include <DgAttributeLinkage.h>
#include <FlatMemStream.h>
#include <OdPlatformStreamer.h>
#include <toString.h>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "helper_functions.h"
#include "data_processor_dwg.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

#pragma optimize("", off)

bool DataProcessorDwg::doDraw(OdUInt32 i, const OdGiDrawable* pDrawable)
{
	OdDbEntityPtr pEntity = OdDbEntity::cast(pDrawable);
	if (!pEntity.isNull())
	{
		// As soon as we get an actual entity, cache the active Layout Id. This
		// can be used to determine when we are back at the top level (out of a
		// block).

		if (context.layoutId.isNull())
		{
			auto layout = OdDbLayout::cast(pEntity->database()->currentLayoutId().safeOpenObject());
			auto layoutBlockId = layout->getBlockTableRecordId();
			context.layoutId = layoutBlockId;
		}

		auto layerId = convertToStdString(toString(pEntity->layerId().getHandle()));
		auto layerName = convertToStdString(toString(pEntity->layer()));

		// If a Block Entity has the default layer assigned, then it appears in
		// the Navisworks tree under the Block Reference's layer. If it has a non
		// default layer assigned, it appears under that Layer in the tree.
		// In both cases the Entity appears under the Block's name in Navisworks,
		// though we do not implement this bit.

		// Make sure we have a layer for the actual DWG layer...

		collector->setLayer(layerId, layerName);


		const OdDbHandle& handle = pEntity->objectId().getHandle();
		OdString sHandle = pEntity->isDBRO() ? toString(handle) : toString(L"non-DbResident");
		std::string handleName = convertToStdString(toString(sHandle));

		// Work out which layer the actual geometry should sit on.

		// Entities that are not members of blocks belong to the Layout's
		// Block Record

		// (An alternative is to check the OdDbBlockTableRecord's getName() for
		// the '*' character, since this prefixes the system block records and
		// is not allowed in user defined block names.)

		if (pEntity->blockId() == context.layoutId)
		{
			context.inBlock = false;
		}

		// This indicates we are entering a Block. The top level block defines
		// the tree node (layer). All subsequent entities, even if they belong
		// to nested blocks, should appear under this node.

		OdDbBlockReferencePtr pBlock = OdDbBlockReference::cast(pDrawable);
		if (!pBlock.isNull() && !context.inBlock)
		{
			context.inBlock = true;
			context.currentBlockReferenceLayerId = layerId;
			context.currentBlockReferenceLayerName = layerName;
			context.currentBlockReferenceHandle = handleName;
			context.currentBlockReferenceName = handleName;

			// If we wish to extract information about the Block in the future, we
			// can get the table entry like so:
			auto record = OdDbBlockTableRecord::cast(pBlock->blockTableRecord().safeOpenObject());

			auto blockName = convertToStdString(record->getName());
		}


		if (context.inBlock) // Layer "0" cannot be renamed, so this is a safe way to check if it's the default layer
		{
			collector->setLayer(context.currentBlockReferenceHandle, context.currentBlockReferenceName, context.currentBlockReferenceLayerId);

			collector->setMeshGroup(context.currentBlockReferenceName);

			std::unordered_map<std::string, std::string> meta;
			meta["Entity Handle::Value"] = convertToStdString(toString(handle));
			collector->setMetadata(handleName, meta);
		}
		else
		{
			collector->setLayer(handleName, handleName, layerId);

			collector->setMeshGroup(handleName);

			std::unordered_map<std::string, std::string> meta;
			meta["Entity Handle::Value"] = convertToStdString(toString(handle));
			collector->setMetadata(handleName, meta);
		}

		// The mesh name should be the user-friendly name of the class type




		collector->setNextMeshName(convertToStdString(sHandle));


		//OdString sClassName = toString(pDrawable->isA());
	}

	return OdGsBaseMaterialView::doDraw(i, pDrawable);
}

void DataProcessorDwg::convertTo3DRepoColor(OdCmEntityColor& color, std::vector<float>& out)
{
	switch (color.colorMethod())
	{
	case OdCmEntityColor::ColorMethod::kByBlock:
	case OdCmEntityColor::ColorMethod::kByLayer:
	case OdCmEntityColor::ColorMethod::kByPen:
		// Currently no special handling is needed for these
		break;

	case OdCmEntityColor::ColorMethod::kByDgnIndex:
	case OdCmEntityColor::ColorMethod::kByACI:
		color.setTrueColor();
		break;

	case OdCmEntityColor::ColorMethod::kForeground:
		color = OdCmEntityColor(255, 255, 255);
		break;
	}

	out.push_back(color.red() / 255.0f);
	out.push_back(color.green() / 255.0f);
	out.push_back(color.blue() / 255.0f);
	out.push_back(1.0f);
}


void DataProcessorDwg::convertTo3DRepoMaterial(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData& materialData,
	MaterialColours& matColors,
	repo_material_t& material,
	bool& missingTexture)
{
	DataProcessor::convertTo3DRepoMaterial(prevCache, materialId, materialData, matColors, material, missingTexture);

	// The Gs superclass supercedes colour data from the material, unless the
	// override flag is set.

	auto traits = effectiveTraits();
	auto deviceColor = traits.trueColor();

	convertTo3DRepoColor(matColors.colorDiffuseOverride ? matColors.colorDiffuse : deviceColor, material.diffuse);
	convertTo3DRepoColor(matColors.colorSpecularOverride ? matColors.colorSpecular : deviceColor, material.specular);

	// For DWGs, we don't set ambient or emissive properties of materials

	material.shininessStrength = 0;
}

void DataProcessorDwg::init(GeometryCollector *const geoCollector)
{
	collector = geoCollector;
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