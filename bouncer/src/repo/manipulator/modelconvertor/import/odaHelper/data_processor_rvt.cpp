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

#include <filesystem>
#include <OdPlatformSettings.h>

#include "vectorise_device_rvt.h"
#include "data_processor_rvt.h"
#include "helper_functions.h"
#include "repo/lib/repo_utils.h"
#include "repo/lib/repo_units.h"
#include "repo/core/model/bson/repo_bson_builder.h"
#include <Database/BmTransaction.h>
#include <Database/BmUnitUtils.h>
#include <Database/Entities/BmRbsSystemType.h>
#include <Base/BmForgeTypeId.h>
#include <Base/BmParameterSet.h>
#include <Base/BmSpecTypeId.h>

using namespace repo::manipulator::modelconvertor::odaHelper;

static const char* ODA_CSV_LOCATION = "ODA_CSV_LOCATION";
static const std::string REVIT_ELEMENT_ID = "Element ID";

//These metadata params are not of interest to users. Do not read.
const std::set<std::string> IGNORE_PARAMS = {
	"RENDER APPEARANCE",
	"RENDER APPEARANCE PROPERTIES",
	"REBAR_NUMBER"
};

bool DataProcessorRvt::tryConvertMetadataEntry(const OdTfVariant& metaEntry, OdBmLabelUtilsPEPtr labelUtils, OdBmParamDefPtr paramDef, OdBm::BuiltInParameter::Enum param, repo::lib::RepoVariant& v)
{
	auto dataType = metaEntry.type();
	switch (dataType) {
		case OdVariant::kVoid: {
			return false;
		}
		case OdVariant::kString: {
			v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(metaEntry.getString());
			break;
		}
		case OdVariant::kBool:
		{
			v = metaEntry.getBool();
			break;
		}
		case OdVariant::kInt8: {
			OdInt8 value = metaEntry.getInt8();
			v = static_cast<int>(value);
			break;
		}
		case OdVariant::kInt16: {
			OdInt16 value = metaEntry.getInt16();
			v = static_cast<int>(value);
			break;
		}
		case OdVariant::kInt32: {
			if (paramDef->getParameterTypeId() == OdBmSpecTypeId::Boolean::kYesNo){
				(metaEntry.getInt32()) ? v = true : v = false;
			} else{
				OdInt32 value = metaEntry.getInt32();
				v = static_cast<int64_t>(value);
			}
			break;
		}
		case OdVariant::kInt64: {
			OdInt64 value = metaEntry.getInt64();
			v = static_cast<int64_t>(value);
			break;
		}
		case OdVariant::kDouble: {
			OdDouble value = metaEntry.getDouble();
			v = static_cast<double>(value);
			break;
		}
		case OdVariant::kAnsiString: {
			v = std::string(metaEntry.getAnsiString().c_str());
			break;
		}
		case OdTfVariant::kDbStubPtr: {
			// A stub is effectively a pointer to another database, or built-in, object. We don't recurse these objects, but will try to extract
			// their names or identities where possible (e.g. if a key pointed to a wall object, we would get the name of the wall object).

			OdDbStub* stub = metaEntry.getDbStubPtr();
			OdBmObjectId bmId = OdBmObjectId(stub);
			if (stub)
			{
				OdDbHandle hdl = bmId.getHandle();
				if (param == OdBm::BuiltInParameter::ELEM_CATEGORY_PARAM || param == OdBm::BuiltInParameter::ELEM_CATEGORY_PARAM_MT)
				{
					if (OdBmObjectId::isRegularHandle(hdl)) // A regular handle points to a database entry; if it is not regular, it is built-in.
					{
						v = std::to_string((OdUInt64)hdl);
					}
					else
					{
						OdBm::BuiltInCategory::Enum builtInValue = static_cast<OdBm::BuiltInCategory::Enum>((OdUInt64)hdl);
						auto category = labelUtils->getLabelFor(OdBm::BuiltInCategory::Enum(builtInValue));
						if (!category.isEmpty()) {
							v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(category);
						}
						else {
							v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(OdBm::BuiltInCategory(builtInValue).toString());
						}
					}
				}
				else
				{
					OdBmObjectPtr bmPtr = bmId.openObject();
					if (!bmPtr.isNull())
					{
						// The object class is unknown - some superclasses we can handle explicitly.

						auto bmPtrClass = bmPtr->isA();
						if (!bmPtrClass->isKindOf(OdBmElement::desc()))
						{
							OdBmElementPtr elem = bmPtr;
							if (elem->getElementName() == OdString::kEmpty) {
								v = std::to_string((OdUInt64)bmId.getHandle());
							}
							else {
								v = repo::manipulator::modelconvertor::odaHelper::convertToStdString(elem->getElementName());
							}
						}
						else
						{
							repoError << "Unsupported metadata value type (class) " << repo::manipulator::modelconvertor::odaHelper::convertToStdString(bmPtrClass->name()) << " currently only OdBmElement's are supported.";
						}
					}
				}
			}
			break;
		}
	default:
		repoWarning << "Unknown Metadata data type encountered in DataProcessorRvt::TryConvertMetadataProperty. Type: " + dataType;
		return false;
	}

	return true;
}


bool DataProcessorRvt::ignoreParam(const std::string& param)
{
	std::string paramUpper = param; // Copy the string to transform it in place
	std::transform(paramUpper.begin(), paramUpper.end(), paramUpper.begin(), ::toupper);
	return IGNORE_PARAMS.find(paramUpper) != IGNORE_PARAMS.end();
}

std::string DataProcessorRvt::getElementName(OdBmElementPtr element)
{
	std::string elName(convertToStdString(element->getElementName()));
	if (!elName.empty())
		elName.append("_");

	elName.append(std::to_string((OdUInt64)element->objectId().getHandle()));

	return elName;
}

std::string DataProcessorRvt::determineTexturePath(const std::string& inputPath)
{
	// Try to extract one valid paths if multiple paths are provided
	auto pathStr = inputPath.substr(0, inputPath.find("|", 0));

	// All Autodesk textures are resolved lowercase, to emulate Windows' case
	// insensitivity
	repo::lib::toLower(pathStr);

	std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
	auto texturePath = std::filesystem::u8path(pathStr); // explictly store the value before calling make_preferred().
	texturePath = texturePath.make_preferred();
	if (repo::lib::doesFileExist(texturePath))
		return texturePath.generic_string();

	// Try to apply absolute path
	std::string env = repo::lib::getEnvString(RVT_TEXTURES_ENV_VARIABLE);
	if (env.empty())
		return std::string();

	auto absolutePath = env / texturePath;
	if (repo::lib::doesFileExist(absolutePath))
		return absolutePath.generic_string();

	// Sometimes the texture path has subdirectories like "./mat/1" remove it and see if we can find it.
	auto altPath = env / texturePath.filename();
	if (repo::lib::doesFileExist(altPath))
		return altPath.generic_string();

	repoDebug << "Failed to find: " << texturePath;
	return std::string();
}

void DataProcessorRvt::initialise(GeometryCollector* collector, OdBmDatabasePtr pDb, OdBmDBViewPtr view, const OdGeMatrix3d& modelToWorld)
{
	this->collector = collector;
	this->view = view;
	this->modelToProjectCoordinates = modelToWorld;
	collector->setUnits(repo::lib::ModelUnits::FEET); // For Revit, the API always uses the internal coordinate system units, which are ft.
}

void DataProcessorRvt::beginViewVectorization()
{
	OdGsBaseMaterialView::beginViewVectorization();
	OdGiGeometrySimplifier::setDrawContext(OdGsBaseMaterialView::drawContext());
	output().setDestGeometry((OdGiGeometrySimplifier&)*this);
	setDrawContextFlags(drawContextFlags(), false);
	setEyeToOutputTransform(getEyeToWorldTransform());
	initLabelUtils();
}

bool DataProcessorRvt::shouldCreateNewLayer(const OdBmElementPtr element)
{
	if (element.isNull())
		return false;

	if (!element->isDBRO())
		return false;

	// In the current version of ODA, each component that is a member of a
	// system is followed by that system as an OdGiDrawable, so we need the
	// geometry to be associated with the previous OdGiDrawable. This is
	// achieved by simply skipping all system drawables (and so not changing
	// the context settings).

	if (element->isKindOf(OdBmRbsSystemType::desc()))
	{
		return false;
	}

	return true;
}

void DataProcessorRvt::draw(const OdGiDrawable* pDrawable)
{
	/*
	* ODA will traverse the tree of drawables, calling draw() for each one. We can't
	* add parameters to draw() so if we want to track the stack we must do it
	* manually. For now though everything needed to build the scene can be retrieved
	* directly from the element.
	*/

	OdBmElementPtr element = OdBmElement::cast(pDrawable);

	if (!element.isNull() && !view->isElementVisible(element))
	{
		return;
	}

	if (!element.isNull() && view->isElementClipped(element, true))
	{
		return;
	}

	// Decide whether we should create a new transformation node for this element and
	// its descendents.

	// This stack frame will hold the reference to the context, if any, until we
	// have unwound back to this frame

	std::unique_ptr<GeometryCollector::Context> ctx;

	if (shouldCreateNewLayer(element))
	{
		ctx = collector->makeNewDrawContext();
	}

	collector->pushDrawContext(ctx.get()); // (Calling push or pop with a nullptr is a no-op)
	OdGsBaseMaterialView::draw(pDrawable);
	collector->popDrawContext(ctx.get());

	if (ctx) // If this frame had it's own context, then extract its meshes (if any)
	{
		// We can skip certain items we know won't be rendered. Ultimately though
		// the visualisation pipeline in ODA (and Revit) is a black-box, and there's
		// no way to know for sure an element will result in geometry on the screen
		// until it starts outputting vertices. Therefore, we only commit transform
		// nodes the first time we actually get meshes for them.
		
		if (ctx->hasMeshes())
		{
			// For Revit files, drawable elements are arranged into layers, not unlike
			// drawings. This means we can get everything from just the element.

			std::string levelName = getLevelName(element, "Layer Default");
			std::string elementName = getElementName(element);

			// These methods create the transformation nodes on-demand

			collector->createLayer(levelName, levelName, {}, {});

			// This call does not necessarily update the transform, so make sure to get
			// the transform explicitly when processing the meshes.

			collector->createLayer(elementName, elementName, levelName, repo::lib::RepoMatrix::translate(ctx->getBounds().min()));

			auto meshes = ctx->extractMeshes(collector->getLayerTransform(elementName).inverse());
			collector->addMeshes(elementName, meshes);

			try
			{
				if (!collector->hasMetadata(elementName)) {
					collector->setMetadata(elementName, fillMetadata(element));
				}
			}
			catch (OdError& er)
			{
				//.. HOTFIX: handle nullPtr exception (reported to ODA)
				repoError << "Caught exception whilst trying to retrieve metadata: " << convertToStdString(er.description());
			}
		}
	}
}

OdGiMaterialItemPtr DataProcessorRvt::fillMaterialCache(
	OdGiMaterialItemPtr prevCache,
	OdDbStub* materialId,
	const OdGiMaterialTraitsData& materialData
) {
	repo::lib::repo_material_t material;
	bool missingTexture = false;

	OdBmObjectId matId(materialId);

	if (!matId.isNull() && matId.isValid()) {
		auto cacheEntry = materialCache.find(matId.getHandle());
		if (cacheEntry != materialCache.end())
		{
			material = cacheEntry->second;
		}
		else
		{
			OdBmMaterialElemPtr materialElem;
			OdBmObjectPtr objectPtr = matId.safeOpenObject();
			if (!objectPtr.isNull())
				materialElem = OdBmMaterialElem::cast(objectPtr);

			fillMaterial(materialElem, materialData, material);
			fillTexture(materialElem, material, missingTexture);

			materialCache[matId.getHandle()] = material;

			if (missingTexture) {
				collector->setMissingTextures();
			}
		}

		collector->setMaterial(material);
	}

	return OdGiMaterialItemPtr();
}

void DataProcessorRvt::triangleOut(const OdInt32* indices, const OdGeVector3d* pNormal)
{
	OdGiMapperItemEntry::MapInputTriangle mapperTriangle;
	GeometryCollector::Face repoTriangle;

	const OdGePoint3d* pVertexDataList = vertexDataList();

	if ((pVertexDataList + indices[0]) != (pVertexDataList + indices[1]) &&
		(pVertexDataList + indices[0]) != (pVertexDataList + indices[2]) &&
		(pVertexDataList + indices[1]) != (pVertexDataList + indices[2]))
	{
		for (int i = 0; i < 3; ++i)
		{
			auto point = pVertexDataList[indices[i]];
			mapperTriangle.inPt[i] = point;
			repoTriangle.push_back(convertToRepoWorldCoordinates(point));
		}
	}
	else
	{
		return; // Triangle is degenerate; this is not fatal (not even technically an error), but we won't store the invisible primitive
	}

	if (isMapperEnabled() && isMapperAvailable()) 
	{
		if (vertexData() && vertexData()->mappingCoords(OdGiVertexData::kAllChannels))
		{
			// Where Uvs are predefined, we need to get them for each vertex from the
			// dedicated array using the indices again...

			OdGiMapperItemEntryPtr mapper = currentMapper(false)->diffuseMapper();
			if (!mapper.isNull()) {
				const OdGePoint3d* predefinedUvCoords = vertexData()->mappingCoords(OdGiVertexData::kAllChannels);
				for (OdInt32 i = 0; i < 3; i++)
				{
					OdGePoint2d uv;
					mapper->mapPredefinedCoords(predefinedUvCoords + indices[i], &uv, 1);
					repoTriangle.push_back(repo::lib::RepoVector2D(uv.x, uv.y));
				}
			}
		}
		else
		{
			// ...Otherwise we can look up uvs directly from the world positions

			if (!currentMapper(true)->diffuseMapper().isNull())
			{
				OdGiMapperItemEntry::MapOutputCoords uvs;
				currentMapper()->diffuseMapper()->mapCoords(mapperTriangle, uvs);
				for (int i = 0; i < 3; i++)
				{
					repoTriangle.push_back(repo::lib::RepoVector2D(uvs.outCoord[i].x, uvs.outCoord[i].y));
				}
			}
		}
	}

	collector->addFace(repoTriangle);
}

void DataProcessorRvt::polylineOut(OdInt32 numPoints, const OdInt32* vertexIndexList)
{
	const auto pVertexDataList = vertexDataList();
	for (int i = 0; i < numPoints - 1; i++)
	{
		collector->addFace({
			convertToRepoWorldCoordinates(pVertexDataList[vertexIndexList[i]]),
			convertToRepoWorldCoordinates(pVertexDataList[vertexIndexList[i + 1]])
		});
	}
}

void DataProcessorRvt::polylineOut(OdInt32 numPoints, const OdGePoint3d* vertexList)
{
	for (OdInt32 i = 0; i < (numPoints - 1); i++)
	{
		collector->addFace({
			convertToRepoWorldCoordinates(vertexList[i]),
			convertToRepoWorldCoordinates(vertexList[i + 1])
		});
	}
}

std::string DataProcessorRvt::getLevelName(OdBmElementPtr element, const std::string& name)
{
	auto levelId = element->getAssocLevelId();
	if (levelId.isValid() && !levelId.isNull()) // (Opening a valid, null pointer will result in an exception)
	{
		auto levelObject = levelId.safeOpenObject();
		if (!levelObject.isNull())
		{
			OdBmLevelPtr lptr = OdBmLevel::cast(levelObject);
			if (!lptr.isNull())
				return std::string(convertToStdString(lptr->getElementName()));
		}
	}

	auto owner = element->getOwningElementId();
	if (owner.isValid() && !owner.isNull())
	{
		auto object = owner.openObject();
		if (!object.isNull())
		{
			auto parentElement = OdBmElement::cast(object);
			if (!parentElement.isNull())
				return getLevelName(parentElement, name);
		}
	}

	return name;
}

void DataProcessorRvt::fillMetadataById(
	OdBmObjectId id,
	std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (id.isNull())
		return;

	OdBmObjectPtr ptr = id.safeOpenObject();

	if (ptr.isNull())
		return;

	fillMetadataByElemPtr(ptr, metadata);
}

void DataProcessorRvt::initLabelUtils() {
	if (!labelUtils) {
		// LabelUtils should be used in embedded mode - where the CSV files are built
		// into TB_ExLabelUtils.
		// This is configured with the EMBEDED_CSV environment variable. Make sure this
		// is not set, as Regular mode will result in runtime errors if the CSVs and
		// Checksums are not also maintained.
		OdBmLabelUtilsPEPtr _labelUtils = OdBmObject::desc()->getX(OdBmLabelUtilsPE::desc());
		labelUtils = (OdBmSampleLabelUtilsPE*)_labelUtils.get();
	}
}

void DataProcessorRvt::processParameter(
	const OdTfVariant& value,
	OdBmObjectId paramId,
	std::unordered_map<std::string, repo::lib::RepoVariant> &metadata,
	const OdBm::BuiltInParameter::Enum &buildInEnum
) {
	OdBmParamElemPtr pParamElem = paramId.safeOpenObject();
	OdBmParamDefPtr pDescParam = pParamElem->getParamDef();
	OdBmForgeTypeId groupId = pDescParam->getGroupTypeId();

	auto metaKey = convertToStdString(pDescParam->getCaption());

	if (!ignoreParam(metaKey)) {
		auto paramGroup = labelUtils->getLabelForGroup(groupId);
		if (!paramGroup.isEmpty()) {
			metaKey = convertToStdString(paramGroup) + "::" + metaKey;
		}

		// Get the label for the units, using the same mechanism as the OdBmSampleLabelUtilsPE::getParamValueAsString.

		auto pAUnits = getUnits(pParamElem->getDatabase());
		auto pFormatOptions = pAUnits->getFormatOptions(pDescParam->getSpecTypeId());
		if (!pFormatOptions.isNull()) {
			auto symbolTypeId = pFormatOptions->getSymbolTypeId();
			auto units = labelUtils->getLabelForSymbol(symbolTypeId);
			if (!units.isEmpty()) {
				metaKey += " (" + convertToStdString(units) + ")";
			}
		}

		repo::lib::RepoVariant v;

		if (tryConvertMetadataEntry(value, labelUtils, pDescParam, buildInEnum, v))
		{
			if (metadata.find(metaKey) != metadata.end() && !boost::apply_visitor(repo::lib::DuplicationVisitor(), metadata[metaKey], v)) {

				repoDebug
					<< "FOUND MULTIPLE ENTRY WITH DIFFERENT VALUES: "
					<< metaKey << "value before: "
					<< boost::apply_visitor(repo::lib::StringConversionVisitor(), metadata[metaKey])
					<< " after: "
					<< boost::apply_visitor(repo::lib::StringConversionVisitor(), v);
			}
			metadata[metaKey] = v;
		}
	}
}

void DataProcessorRvt::fillMetadataByElemPtr(
	OdBmElementPtr element,
	std::unordered_map<std::string, repo::lib::RepoVariant>& outputData)
{
	if (!labelUtils) return;

	OdBmParameterSet aParams;
	element->getListParams(aParams);

	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;

	for (const auto& entry : aParams.getBuiltInParamsIterator()) {
		std::string builtInName = convertToStdString(OdBm::BuiltInParameter(entry).toString());

		//.. HOTFIX: handle access violation exception (reported to ODA)
		// https://forum.opendesign.com/showthread.php?25064-Access-Violation-in-OdBmElement-getParam&p=102079#post102079
		// https://account.opendesign.com/support/issue-tracking/BIM-7094
		if (ignoreParam(builtInName)) continue;

		auto paramId = element->database()->getObjectId(entry);
		if (paramId.isValid() && !paramId.isNull()) {

			OdTfVariant value;
			if (element->getParam(paramId, value) == eOk) {
				processParameter(value, paramId, metadata, entry);
			}
		}
	}

	// This snippet gets user parameter values. Elements may have lots of parameters
	// without values, if say, the project has lots of shared parameters. Since we
	// filter out parameters without values anyway, this is much more efficient than
	// checking each parameter in the user params list one by one, as most of them
	// may end up filtered out anyway.

	OdBmMap<OdBmObjectId, OdTfVariant> parameters;
	element->getParameters(parameters);
	for (const auto &entry : parameters) {
		processParameter(entry.second, entry.first, metadata,
			//A dummy entry, as long as it's not ELEM_CATEGORY_PARAM_MT or ELEM_CATEGORY_PARAM it's not utilised.
			OdBm::BuiltInParameter::Enum::ACTUAL_MAX_RIDGE_HEIGHT_PARAM);
	}

	if (metadata.size()) {
		outputData.insert(metadata.begin(), metadata.end());
	}
}

std::unordered_map<std::string, repo::lib::RepoVariant> DataProcessorRvt::fillMetadata(OdBmElementPtr element)
{
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
	metadata[REVIT_ELEMENT_ID] = static_cast<int64_t>((OdUInt64)element->objectId().getHandle());

	try
	{
		fillMetadataByElemPtr(element, metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to fetch metadata by element pointer " << convertToStdString(er.description()) << " Code: " << er.code();
	}

	try
	{
		fillMetadataById(element->getFamId(), metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to get metadata by family ID: " << convertToStdString(er.description()) << " Code: " << er.code();
	}

	try
	{
		fillMetadataById(element->getTypeID(), metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to get metadata by Type ID: " << convertToStdString(er.description()) << " Code: " << er.code();
	}

	try
	{
		fillMetadataById(element->getHeaderCategoryId(), metadata);
	}
	catch (OdError& er)
	{
		repoTrace << "Caught exception whilst trying to get metadata by category ID: " << convertToStdString(er.description()) << " Code: " << er.code();
	}

	return metadata;
}

std::vector<float> toRepoColour(const OdGiMaterialColor& c)
{
	return { c.color().red() / 255.0f, c.color().green() / 255.0f, c.color().blue() / 255.0f, 1.0f };
}

void DataProcessorRvt::fillMaterial(OdBmMaterialElemPtr materialPtr, const OdGiMaterialTraitsData& materialData, repo::lib::repo_material_t& material)
{
	OdGiMaterialColor colour;

	materialData.shadingDiffuse(colour);
	material.diffuse = toRepoColour(colour);

	materialData.shadingSpecular(colour);
	material.specular = toRepoColour(colour);

	materialData.shadingAmbient(colour);
	material.ambient = toRepoColour(colour);

	OdGiMaterialMap map;
	materialData.emission(colour, map);
	material.emissive = toRepoColour(colour);

	double opacity;
	materialData.shadingOpacity(opacity);
	material.opacity = opacity;

	OdGiMaterialColor specularColor; OdGiMaterialMap specularMap; double glossFactor;
	materialData.specular(specularColor, specularMap, glossFactor);
	material.shininessStrength = glossFactor;

	if (!materialPtr.isNull())
		material.shininess = (float)materialPtr->getMaterial()->getShininess() / 255.0f;
}

void DataProcessorRvt::fillTexture(OdBmMaterialElemPtr materialPtr, repo::lib::repo_material_t& material, bool& missingTexture)
{
	missingTexture = false;

	if (materialPtr.isNull())
		return;

	OdBmMaterialPtr matPtr = materialPtr->getMaterial();
	OdBmAssetPtr textureAsset = matPtr->getAsset();

	if (textureAsset.isNull())
		return;

	OdBmAssetPtr bitmapAsset;
	OdString textureFileName;
	OdResult es = OdBmAppearanceAssetHelper::getTextureAsset(textureAsset, bitmapAsset);

	if ((es != OdResult::eOk) || (bitmapAsset.isNull()))
		return;

	OdBmUnifiedBitmapSchemaHelper textureHelper(bitmapAsset);
	es = textureHelper.getTextureFileName(textureFileName);

	if (es != OdResult::eOk)
		return;

	std::string textureName(convertToStdString(textureFileName));
	std::string validTextureName = determineTexturePath(textureName);
	if (validTextureName.empty() && !textureName.empty())
		missingTexture = true;

	if (!missingTexture) {
		// If a material property uses a map, then set the colour to white, as our default
		// behaviour is to multiply in the shader, and Revit's is to replace it.
		// (Revit's Appearance dialog does have a Tint option, but its more for use during
		// ray-tracing.)
		material.diffuse = { 1,1,1 };
	}

	material.texturePath = validTextureName;
}

OdBmAUnitsPtr DataProcessorRvt::getUnits(OdBmDatabasePtr database)
{
	OdBmFormatOptionsPtrArray aFormatOptions;
	OdBmUnitsTrackingPtr pUnitsTracking = database->getAppInfo(OdBm::ManagerType::UnitsTracking);
	OdBmUnitsElemPtr pUnitsElem = pUnitsTracking->getUnitsElemId().safeOpenObject();
	OdBmAUnitsPtr ptrAUnits = pUnitsElem->getUnits().get();
	return ptrAUnits;
}

repo::lib::RepoVector3D64 DataProcessorRvt::convertToRepoWorldCoordinates(OdGePoint3d p)
{
	return toRepoVector(p.transformBy(modelToProjectCoordinates));
}
