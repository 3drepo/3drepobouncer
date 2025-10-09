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

// 3d repo bouncer

#include "helper_functions.h"
#include "data_processor_nwd.h"

#include "repo/manipulator/modelutility/repo_scene_builder.h"
#include "repo/manipulator/modelconvertor/import/odaHelper/repo_mesh_builder.h"
#include "repo/core/model/bson/repo_node_transformation.h"
#include "repo/core/model/bson/repo_bson_factory.h"

// ODA
#include <OdaCommon.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <StaticRxObject.h>
#include <NwHostAppServices.h>
#include <NwDatabase.h>
#include <toString.h>
#include <ExPrintConsole.h>

#include <NwModelItem.h>
#include <NwMaterial.h>
#include <NwComponent.h>
#include <NwFragment.h>
#include <NwGeometryCircle.h>
#include <NwGeometryLineSet.h>
#include <NwGeometryShell.h>
#include <NwGeometryPointSet.h>
#include <NwGeometryText.h>
#include <NwGeometryCylinder.h>
#include <NwTexture.h>
#include <NwColor.h>
#include <NwBackgroundElement.h>
#include <NwViewpoint.h>
#include <NwModel.h>
#include <NwProperty.h>
#include <NwPartition.h>
#include <NwCategoryConversionType.h>
#include <NwURL.h>
#include <NwVerticesData.h>

#include <Attribute/NwAttribute.h>
#include <Attribute/NwPropertyAttribute.h>
#include <Attribute/NwGraphicMaterialAttribute.h>
#include <Attribute/NwGuidAttribute.h>
#include <Attribute/NwMaterialAttribute.h>
#include <Attribute/NwNameAttribute.h>
#include <Attribute/NwBinaryAttribute.h>
#include <Attribute/NwPresenterMaterialAttribute.h>
#include <Attribute/NwPresenterTextureSpaceAttribute.h>
#include <Attribute/NwPublishAttribute.h>
#include <Attribute/NwTextAttribute.h>
#include <Attribute/NwTransAttribute.h>
#include <Attribute/NwTransformAttribute.h>
#include <Attribute/NwTransRotationAttribute.h>
#include <Attribute/NwUInt64Attribute.h>
#include <Attribute/NwURLAttribute.h>
#include <NwDataProperty.h>

#include <NwGiContext.h>
#include <Gi/GiBaseVectorizer.h>
#include <Gi/GiGeometrySimplifier.h>

// boost
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace repo::lib;
using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace repo::core::model;

static std::vector<std::string> ignoredCategories = { "Material", "Geometry", "Autodesk Material", "Revit Material" };
static std::vector<std::string> ignoredKeys = { "Item::Source File Name" };
static std::vector<std::string> keysForPathSanitation = { "Item::Source File", "Item::File Name" };
static std::string sElementIdKey = "Element ID::Value";

// The Vectorizer and Simplifier are used to tessellate objects that are not
// stored as meshes in the Nwd, such as text.

class Simplifier : public OdGiGeometrySimplifier
{
public:
	virtual void triangleOut(const OdInt32* indices,
		const OdGeVector3d* pNormal) override
	{
		auto vertices = (repo::lib::RepoVector3D64*)vertexDataList();
		builder->addFace(
			RepoMeshBuilder::face({
			vertices[indices[0]],
			vertices[indices[1]],
			vertices[indices[2]]
		},
		*(repo::lib::RepoVector3D64*)pNormal
		));
	}

	void setMeshBuilder(RepoMeshBuilder& builder)
	{
		this->builder = &builder;
	}

private:
	RepoMeshBuilder* builder;
};

class OdVectorizer : protected OdGiBaseVectorizer
{
private:
	OdGiContextForNwDatabasePtr pContext;
	Simplifier simplifier;

public:
	ODRX_HEAP_OPERATORS();
	virtual ~OdVectorizer() = default;

	static OdSharedPtr<OdVectorizer> createObject(OdNwDatabasePtr pNwDb)
	{
		auto context = OdGiContextForNwDatabase::createObject();
		context->setDatabase(pNwDb);
		auto pThis = new OdStaticRxObject<OdVectorizer>;
		pThis->setContext(context);
		return pThis;
	}

	void setContext(OdGiContextForNwDatabasePtr context)
	{
		pContext = context; // context is a smart pointer so this member keeps it alive until we're done with it
		OdGiBaseVectorizer::setContext(pContext);
		m_pModelToEyeProc->setDrawContext(drawContext());
		output().setDestGeometry(simplifier);
		simplifier.setDrawContext(drawContext());
	}

	void draw(const OdGiDrawable* drawable, RepoMeshBuilder& builder)
	{
		simplifier.setMeshBuilder(builder);
		OdGiBaseVectorizer::draw(drawable);
	}
};

using OdVectorizerPtr = OdSharedPtr<OdVectorizer>;

struct RepoNwTraversalContext {
	OdNwModelItemPtr layer;
	OdNwPartitionPtr partition;
	repo::manipulator::modelutility::RepoSceneBuilder* sceneBuilder = nullptr;
	std::shared_ptr<repo::core::model::TransformationNode> parentNode;
	std::unordered_map<std::string, repo::lib::RepoVariant> parentMetadata;
	OdGeMatrix3d transform;
	OdVectorizerPtr vectorizer;
};

repo::lib::RepoVector3D64 convertPoint(OdGePoint3d pnt, OdGeMatrix3d& transform)
{
	pnt.transformBy(transform);
	return repo::lib::RepoVector3D64(pnt.x, pnt.y, pnt.z);
};

repo::lib::RepoVector3D64 convertPoint(OdGePoint3d pnt)
{
	return repo::lib::RepoVector3D64(pnt.x, pnt.y, pnt.z);
};

repo::lib::RepoVector2D convertPoint(OdGePoint2d pnt)
{
	return repo::lib::RepoVector2D(pnt.x, pnt.y);
};

// These next methods explicitly ignore the alpha component of the Nw colours,
// because the material *should* have the transparency accessible via a
// dedicated member. If we get issues with materials though, this should be
// re-evaluated.

void convertColor(OdNwColor color, repo_color3d_t& dest)
{
	dest.r = color.R();
	dest.g = color.G();
	dest.b = color.B();
}

void convertColor(OdString color, repo_color3d_t& dest)
{
	try
	{
		typedef boost::tokenizer<boost::char_separator<OdChar>> tokenizer;
		boost::char_separator<OdChar> sep(OD_T(","));
		auto stringified = convertToStdString(color);
		tokenizer tokens(stringified, sep);
		std::vector<float> components;
		for (tokenizer::iterator iter = tokens.begin(); iter != tokens.end(); iter++) {
			auto token = *iter;
			boost::trim(token);
			auto component = boost::lexical_cast<float>(token);
			components.push_back(component / 255.0);
		}
		dest = components;
	}
	catch (...)
	{
		repoError << "Exception parsing string " << convertToStdString(color) << " to a color.";
	}
}

std::string sanitiseString(std::string key, std::string value)
{
	if (std::find(keysForPathSanitation.begin(), keysForPathSanitation.end(), key) != keysForPathSanitation.end()) {
		return boost::filesystem::path(value).filename().string();
	}
	else {
		return value;
	}
}

void setMetadataValueVariant(const std::string& category, const std::string& key, const repo::lib::RepoVariant& value, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	std::string metaKey = category + "::" + (key.empty() ? std::string("Value") : key);
	metadata[metaKey] = value;
}

void setMetadataValue(const std::string& category, const std::string& key, const bool& b, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	repo::lib::RepoVariant v = b;
	setMetadataValueVariant(category, key, v, metadata);
}

void setMetadataValue(const std::string& category, const std::string& key, const double& d, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	repo::lib::RepoVariant v = d;
	setMetadataValueVariant(category, key, v, metadata);
}

void setMetadataValue(const std::string& category, const OdString& key, const OdUInt64& uint, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	std::string keyString = convertToStdString(key);
	repo::lib::RepoVariant v = static_cast<int64_t>(uint); // Potentially losing precision here, but mongo does not accept uint64
	setMetadataValueVariant(category, keyString, v, metadata);
}

void setMetadataValue(const std::string& category, const OdString& key, const OdString& string, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	std::string keyString = convertToStdString(key);
	std::string strValue = repo::manipulator::modelconvertor::odaHelper::convertToStdString(string);
	strValue = sanitiseString(keyString, strValue);
	repo::lib::RepoVariant v = strValue;
	setMetadataValueVariant(category, keyString, v, metadata);
}

bool DataProcessorNwd::tryConvertMetadataProperty(std::string key, OdNwDataPropertyPtr& metaProperty, repo::lib::RepoVariant& v)
{
	auto dataType = metaProperty->getDataType();
	switch (dataType)
	{
		case NwDataType::dt_NONE: {
			return false;
		}
		case NwDataType::dt_DOUBLE: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			v = odvar.getDouble();
			break;
		}
		case NwDataType::dt_INT32: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			v = static_cast<int64_t>(odvar.getInt32()); // Incoming is long, convert it to long long since int won't fit.
			break;
		}
		case NwDataType::dt_BOOL: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			v = odvar.getBool();
			break;
		}
		case NwDataType::dt_DISPLAY_STRING: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			OdString value = odvar.getString();
			std::string strValue = repo::manipulator::modelconvertor::odaHelper::convertToStdString(value);
			strValue = sanitiseString(key, strValue);
			v = strValue;
			break;
		}
		case NwDataType::dt_DATETIME: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			time_t timePosix = static_cast<time_t>(odvar.getUInt64());
			tm* timeTm = localtime(&timePosix);
			v = *timeTm;
			break;
		}
		case NwDataType::dt_DOUBLE_LENGTH: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			v = odvar.getMeasuredDouble();
			break;
		}
		case NwDataType::dt_DOUBLE_ANGLE: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			v = odvar.getMeasuredDouble();
			break;
		}
		case NwDataType::dt_NAME: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			OdRxObjectPtr ptr = odvar.getRxObjectPtr();
			OdNwNamePtr namePtr = static_cast<OdNwNamePtr>(ptr);
			OdString displayNameOd = namePtr->getDisplayName();
			std::string displayName = repo::manipulator::modelconvertor::odaHelper::convertToStdString(displayNameOd);
			v = displayName;
			break;
		}
		case NwDataType::dt_IDENTIFIER_STRING: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			OdString value = odvar.getString();
			std::string strValue = repo::manipulator::modelconvertor::odaHelper::convertToStdString(value);
			strValue = sanitiseString(key, strValue);
			v = strValue;
			break;
		}
		case NwDataType::dt_DOUBLE_AREA: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			v = odvar.getMeasuredDouble();
			break;
		}
		case NwDataType::dt_DOUBLE_VOLUME: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			v = odvar.getMeasuredDouble();
			break;
		}
		case NwDataType::dt_POINT2D: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			OdGePoint2d point = odvar.getPoint2d();
			std::string str = std::to_string(point.x) + ", " + std::to_string(point.y);
			v = str;
			break;
		}
		case NwDataType::dt_POINT3D: {
			OdNwVariant odvar;
			metaProperty->getValue(odvar);
			OdGePoint3d point = odvar.getPoint3d();
			std::string str = std::to_string(point.x) + ", " + std::to_string(point.y) + ", " + std::to_string(point.z);
			v = str;
			break;
		}
		default: {
			// All other cases can not be handled by this converter.
			repoWarning << "Unknown Metadata data type encountered in DataProcessorNwd::tryConvertMetadataProperty. Type: " + dataType;
			return false;
		}
	}

	return true;
}

// Materials are defined at the Component level. Components contain different types
// geometric primitive, all with the same material.

void processMaterial(OdNwComponentPtr pComp, repo_material_t& repoMaterial)
{
	// BimNv has two types of material, that used in Realistic Mode (i.e.textured)
	// and Shaded Mode.
	// getMaterial() is for realistic (and will hold the texture), while
	// getOriginalMaterial() corresponds to Shaded mode.
	// https://forum.opendesign.com/showthread.php?20568-How-to-get-the-LcOaExMaterial-properties

	// We currently process the Shaded view, to match the plugin.

	auto materialId = pComp->getOriginalMaterialId();
	if (!materialId.isNull())
	{
		OdNwMaterialPtr pMaterial = materialId.safeOpenObject();

		convertColor(pMaterial->getDiffuse(), repoMaterial.diffuse);
		convertColor(pMaterial->getAmbient(), repoMaterial.ambient);
		convertColor(pMaterial->getEmissive(), repoMaterial.emissive);
		convertColor(pMaterial->getSpecular(), repoMaterial.specular);

		repoMaterial.shininess = pMaterial->getShininess();
		repoMaterial.opacity = 1 - pMaterial->getTransparency();
	}

	// Textures are ignored for now, but if a material has a texture, it will be
	// an OdNwTexture subclass instance, which we can check for and convert to,
	// to access the texture properties.
	//
	// e.g. if (pMaterial->isA() == OdNwTexture::desc()) { }
	//
	// Unlike BimRv, the texture references in NWDs are not simple filenames, but
	// GUIDs into the shared Autodesk material database. A complete copy of the
	// database is required to resolve textures, and if this is not set correctly
	// returned texture paths will be null.
	//
	// e.g.
	// svcs.setTextureDirectoryPath(OD_T("C:\\Program Files(x86)\\Common Files\\Autodesk Shared\\Materials\\Textures"), false);
	// pNwDb->setTextureFolder(OD_T("C:\\3drepo\\bouncer\\revit_materials_edited"));
	//
	// Note that only Realistic materials have textures, so get the Realistic
	// material with getMaterialId() (instead of Shaded with getOriginalMaterialId()
	// before checking.

	// Beyond the above, it is possible to iterate over all the material properties
	// using pMaterial->getProperties(). These don't provide any higher level
	// information (e.g. texture filenames) but do provide more details that could
	// be useful (e.g. names and descriptions)
}

void processAttributes(OdNwModelItemPtr modelItemPtr, RepoNwTraversalContext context, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (modelItemPtr.isNull()) {
		return;
	}

	// This method extracts all the metadata for the model Item node, and stores
	// in in the metadata map as text, in the form it will be displayed on the
	// web.

	// The Attributes correspond to the Property Categories in Navisworks.
	// There doesn't appear to be an attribute type for the Item Tab (LcOaNode).
	// We can infer these parameters from the OdNwModelItem, and its positition
	// in the scene graph.

	auto ItemName = modelItemPtr->getDisplayName(); // e.g. "Newspaper"
	auto ItemType = modelItemPtr->getClassDisplayName();  // e.g. "Furniture: Newspaper: Newspaper"
	auto ItemInternalType = modelItemPtr->getClassName(); // e.g. "LcRevitInstance"
	auto ItemHidden = modelItemPtr->isHidden();

	// There are no search results suggesting any methods to get the 'required' flag.
	// This flag is set in Navisworks to indicate an element should always be drawn.
	// It is not required to store it.
	//	auto ItemRequired = modelItemPtr->?

	// For the Layer, we can use the scene graph hierarchy.
	auto ItemLayer = context.layer->getDisplayName();

	if (!OdNwPartition::cast(context.layer).isNull())
	{
		ItemLayer = OdString(boost::filesystem::path(ItemLayer).filename().c_str());
	}

	auto ItemGuid = modelItemPtr->getInstanceGuid().toString();
	auto hasGuid = modelItemPtr->getInstanceGuid() != OdGUID::kNull;

	// Print the custom Item block to the metadata


	setMetadataValue("Item", "Name", ItemName, metadata);
	setMetadataValue("Item", "Type", ItemType, metadata);
	setMetadataValue("Item", "Internal Type", ItemInternalType, metadata);
	setMetadataValue("Item", "Hidden", ItemHidden, metadata);
	setMetadataValue("Item", "Layer", ItemLayer, metadata);
	if (hasGuid) {
		setMetadataValue("Item", "GUID", ItemGuid, metadata);
	}

	// The source file name is contained within the OdNwPartition entry in the
	// scene graph, which segments branches. Note that this will contain the
	// entire path (the path where the NWD is stored, if an NWD).

	if (!context.partition.isNull()) {
		setMetadataValue("Item", "Source File", context.partition->getSourceFileName(), metadata);
	}
	else {
		repoError << "Unknown partition (model source file) for " << convertToStdString(ItemName) << " this should never happen.";
	}

	// Now iterate the remaining properties...

	OdArray<OdNwAttributePtr> attributes;
	modelItemPtr->getAttributes(attributes, 0);
	for (unsigned int i = 0; i < attributes.length(); i++)
	{
		auto attribute = attributes[i];
		if (attribute.isNull()) {
			continue;
		}

		// getClassDisplayName() is the human-readable label that is seen in the
		// Navisworks GUI (on the Tabs)
		// getClassName() is the internal name. This can also be seen on the tabs by
		// activating the 'Show Property Internal Names' option in
		//     Navisworks -> Options -> Interface -> Developer.

		auto name = convertToStdString(attribute->getClassName());
		auto category = convertToStdString(attribute->getClassDisplayName());

		// Ignore some specific Categories based on their name, regardless of
		// Attribute Type

		if (std::find(ignoredCategories.begin(), ignoredCategories.end(), category) != ignoredCategories.end()) {
			continue;
		}

		// OdNwAttributes don't self-identify their type like the OdNwFragments.
		// Cast to one of the sub-classes to determine the type.
		// https://docs.opendesign.com/bimnv/OdNwAttribute.html

		// The OdNwPropertyAttribute is the typical block that contains key-value
		// pairs in the Tabs in Navisworks. These hold most of the propreties.

		auto propertiesAttribute = OdNwPropertyAttribute::cast(attribute);
		if (!propertiesAttribute.isNull())
		{
			OdArray<OdNwDataPropertyPtr> properties;
			propertiesAttribute->getProperties(properties);

			for (unsigned int j = 0; j < properties.length(); j++)
			{
				auto& prop = properties[j];
				auto key = convertToStdString(prop->getDisplayName());

				repo::lib::RepoVariant v;
				if (DataProcessorNwd:: tryConvertMetadataProperty(key, prop, v))
					setMetadataValueVariant(category, key, v, metadata);
			}
		}

		// Translation and Rotation attribute
		auto transRotationAttribute = OdNwTransRotationAttribute::cast(attribute);
		if (!transRotationAttribute.isNull())
		{
			auto translation = transRotationAttribute->getTranslation();
			auto rotation = transRotationAttribute->getRotation();

			setMetadataValue("Transform", "Rotation:X", rotation.x, metadata);
			setMetadataValue("Transform", "Rotation:Y", rotation.y, metadata);
			setMetadataValue("Transform", "Rotation:Z", rotation.z, metadata);
			setMetadataValue("Transform", "Rotation:W", rotation.w, metadata);
			setMetadataValue("Transform", "Translation:X", translation.x, metadata);
			setMetadataValue("Transform", "Translation:Y", translation.y, metadata);
			setMetadataValue("Transform", "Translation:Z", translation.z, metadata);
		}

		// Transform matrix attribute
		auto transformAttribute = OdNwTransformAttribute::cast(attribute);
		if (!transformAttribute.isNull())
		{
			setMetadataValue("Transform", "Reverses", transformAttribute->isReverse(), metadata);
			auto matrix = transformAttribute->getTransform();

			OdGeVector3d translation(matrix.entry[0][3], matrix.entry[1][3], matrix.entry[2][3]);
			setMetadataValue("Transform", "Translation:X", translation.x, metadata);
			setMetadataValue("Transform", "Translation:Y", translation.y, metadata);
			setMetadataValue("Transform", "Translation:Z", translation.z, metadata);

			//Todo: extract rotation (in angle axis format) and scale
		}

		// Translation attribute
		auto transAttribute = OdNwTransAttribute::cast(attribute);
		if (!transAttribute.isNull())
		{
			auto translation = transAttribute->getTranslation();
			setMetadataValue("Transform", "Translation:X", translation.x, metadata);
			setMetadataValue("Transform", "Translation:Y", translation.y, metadata);
			setMetadataValue("Transform", "Translation:Z", translation.z, metadata);
		}

		// (Note The Guid in the Item block is not an attribute, but rather is retrieved using getInstanceGuid(). This will be something else.)

		auto guidAttribute = OdNwGuidAttribute::cast(attribute);
		if (!guidAttribute.isNull())
		{
			setMetadataValue(category, guidAttribute->getDisplayName(), guidAttribute->getGuid().toString(), metadata);
		}

		// (Note the "Element Id::Value" (LcOaNat64AttributeValue) key is stored as a UInt64 Attribute.)

		auto uint64Attribute = OdNwUInt64Attribute::cast(attribute);
		if (!uint64Attribute.isNull())
		{
			setMetadataValue(category, uint64Attribute->getDisplayName(), uint64Attribute->getValue(), metadata);
		}

		auto materialAttribute = OdNwMaterialAttribute::cast(attribute);
		if (!materialAttribute.isNull())
		{
			auto value = materialAttribute->getDisplayName();
			setMetadataValue("Item", "Material", value, metadata);
		}

		auto binaryAttribute = OdNwBinaryAttribute::cast(attribute);
		if (!binaryAttribute.isNull())
		{
			OdBinaryData data;
			binaryAttribute->getData(data);
			auto asString = OdString((OdChar*)data.asArrayPtr(), data.length());
			setMetadataValue(category, binaryAttribute->getDisplayName(), asString, metadata);
		}

		auto urlAttribute = OdNwURLAttribute::cast(attribute);
		if (!urlAttribute.isNull())
		{
			OdArray<OdNwURLPtr> urls;
			urlAttribute->getURLs(urls);
			OdString combined;
			for (auto url : urls) {
				if (!url.isNull()) {
					combined += url->getURL() + ";";
				}
			}
			setMetadataValue(category, urlAttribute->getDisplayName(), combined, metadata);
		}

		auto textAttribute = OdNwTextAttribute::cast(attribute);
		if (!textAttribute.isNull())
		{
			setMetadataValue(category, textAttribute->getDisplayName(), textAttribute->getText(), metadata);
		}

		//Other Attributes include:
		//	OdNwGraphicMaterial
		//	OdNwName
		//	OdNwPresenterMaterial
		//	OdNwPresenterTextureSpace
		//	OdNwPublish
		// These are currently ignored.
	}


	// And remove any that should be ignored (at the end, so we don't have to
	// check every key every time when looking for just a couple..)

	for (auto key : ignoredKeys) {
		metadata.erase(key);
	}
}

void addTriangleData(
	const OdNwVerticesDataPtr aVertexesData,
	const OdNwTriangleIndexes* aTrianglesBegin,
	const OdNwTriangleIndexes* aTrianglesEnd,
	OdGeMatrix3d& transformMatrix,
	RepoMeshBuilder& meshBuilder)
{
	const OdGePoint3dArray& aVertices = aVertexesData->getVertices();
	const OdGeVector3dArray& aNormals = aVertexesData->getNormals();
	const OdGePoint2dArray& aUvs = aVertexesData->getTexCoords();

	for (auto triangle = aTrianglesBegin; triangle != aTrianglesEnd; triangle++)
	{
		RepoMeshBuilder::face face;

		face.setVertices({
			convertPoint(aVertices[triangle->pointIndex1], transformMatrix),
			convertPoint(aVertices[triangle->pointIndex2], transformMatrix),
			convertPoint(aVertices[triangle->pointIndex3], transformMatrix)
			});

		if (aUvs.length())
		{
			face.setUvs({
				convertPoint(aUvs[triangle->pointIndex1]),
				convertPoint(aUvs[triangle->pointIndex2]),
				convertPoint(aUvs[triangle->pointIndex3])
				});
		}

		meshBuilder.addFace(face);
	}
}

OdResult processGeometry(OdNwModelItemPtr pNode, RepoNwTraversalContext context)
{
	// OdNwComponent instances contain geometry in the form of a set of OdNwFragments.
	// Each OdNwFragments can have a different primitive type (lines, triangles, etc).
	// Materials are defined at the OdNwComponent level.

	OdNwObjectId compId = pNode->getGeometryComponentId();
	if (compId.isNull())
	{
		return eOk;
	}

	OdNwComponentPtr pComp = OdNwComponent::cast(compId.safeOpenObject());
	if (pComp.isNull())
	{
		return eOk;
	}

	// BimNv can have multiple material representations. The getMaterialId member
	// refers to the Autodesk Material Properties. Other properties are represented
	// in attributes.
	// Parse the member first as a back-up, then override any settings that we
	// know how to handle explicitly.

	repo_material_t repoMaterial;
	processMaterial(pComp, repoMaterial);

	RepoMeshBuilder meshBuilder({ context.parentNode->getSharedID() }, -context.sceneBuilder->getWorldOffset(), repoMaterial);
	OdNwObjectIdArray aCompFragIds;
	pComp->getFragments(aCompFragIds);
	for (OdNwObjectIdArray::const_iterator itFrag = aCompFragIds.begin(); itFrag != aCompFragIds.end(); ++itFrag)
	{
		if (!(*itFrag).isNull())
		{
			OdNwFragmentPtr pFrag = OdNwFragment::cast((*itFrag).safeOpenObject());
			OdGeMatrix3d transformMatrix = context.transform * pFrag->getTransformation();
			OdNwObjectId geometryId = pFrag->getGeometryId(); // Returns an object ID of the base class OdNwGeometry object with geometry data.

			if (geometryId.isNull() || !geometryId.isValid())
			{
				continue;
			}

			OdNwObjectPtr pGeometry = geometryId.safeOpenObject();

			// If the fragment has geometry, see if it is of a type that we support
			if (pGeometry->isA() == OdNwGeometryLineSet::desc())
			{
				OdNwGeometryLineSetPtr pGeomertyLineSet = OdNwGeometryLineSet::cast(pGeometry);
				if (!pGeomertyLineSet.isNull())
				{
					// BimNv lines also have colours, but these are not supported yet.

					OdNwVerticesDataPtr aVertexesData = pGeomertyLineSet->getVerticesData();
					OdGePoint3dArray aVertexes = aVertexesData->getVertices();
					OdUInt16Array aVertexPerLine = pGeomertyLineSet->getVerticesCountPerLine();

					// Each entry in aVertexPerLine corresponds to one line, which is a vertex strip. This snippet
					// converts each strip into a set of 2-vertex line segments.

					auto index = 0;
					for (auto line = aVertexPerLine.begin(); line != aVertexPerLine.end(); line++)
					{
						for (auto i = 0; i < (*line - 1); i++)
						{
							meshBuilder.addFace(RepoMeshBuilder::face({
								convertPoint(aVertexes[index + 0], transformMatrix),
								convertPoint(aVertexes[index + 1], transformMatrix)
								}));
							index++;
						}
						index++;
					}

					continue;
				}
			}
			else if (pGeometry->isA() == OdNwGeometryShell::desc())
			{
				OdNwGeometryShellPtr pGeometryMesh = OdNwGeometryShell::cast(pGeometry);
				if (!pGeometryMesh.isNull())
				{
					auto triangles = pGeometryMesh->getTriangles();
					auto vertices = pGeometryMesh->getVerticesData();
					addTriangleData(vertices, triangles.begin(), triangles.end(), transformMatrix, meshBuilder);
					continue;
				}
			}
			else if (pGeometry->isA() == OdNwGeometryCircle::desc())
			{
				OdNwGeometryCirclePtr pGeometryEllipse = OdNwGeometryCircle::cast(pGeometry);
				if (!pGeometryEllipse.isNull())
				{
					auto shell = pGeometryEllipse->toShell(8);
					addTriangleData(shell.first, shell.second.begin(), shell.second.end(), transformMatrix, meshBuilder);
					continue;
				}
			}
			else if (pGeometry->isA() == OdNwGeometryCylinder::desc())
			{
				OdNwGeometryCylinderPtr pGeometryCylinder = OdNwGeometryCylinder::cast(pGeometry);
				if (!pGeometryCylinder.isNull())
				{
					auto shell = pGeometryCylinder->toShell(8);
					addTriangleData(shell.first, shell.second.begin(), shell.second.end(), transformMatrix, meshBuilder);
					continue;
				}
			}
			else if (pGeometry->isA() == OdNwGeometryText::desc())
			{
				OdNwGeometryTextPtr pGeometryText = OdNwGeometryText::cast(pGeometry);
				if (!pGeometryText.isNull())
				{
					context.vectorizer->draw(pNode, meshBuilder);
				}
			}

			// The full set of types that the OdNwGeometry could be are described
			// here: https://docs.opendesign.com/bimnv/OdNwGeometry.html
		}
	}

	std::vector<repo::core::model::MeshNode> nodes;
	meshBuilder.extractMeshes(nodes);
	for (auto& mesh : nodes)
	{
		mesh.setMaterial(meshBuilder.getMaterial());
		context.sceneBuilder->addNode(mesh);
	}

	return eOk;
}

bool isInstanced(OdNwModelItemPtr pNode)
{
	if (pNode.isNull()) {
		return false;
	}
	if (pNode->getIcon() == NwModelItemIcon::INSERT_GEOMETRY) {
		return true;
	}
	if (pNode->getIcon() == NwModelItemIcon::INSERT_GROUP) {
		return true;
	}
	return false;
}

bool isCollection(OdNwModelItemPtr pNode)
{
	if (pNode.isNull()) {
		return false;
	}
	if (pNode->getIcon() == NwModelItemIcon::COLLECTION) {
		return true;
	}
	if (pNode->getIcon() == NwModelItemIcon::GROUP) {
		return true;
	}
	return false;
}

// Traverses the scene graph recursively. Each invocation of this method operates
// on one OdNwModelItem, which is the basic OdNwObject that makes up the geometry
// scene/the tree in the Standard View.
// As the scene is traversed we pass nodes that delimit branches, e.g. nodes that
// define the start of whole files. These stateful properties are stored in the
// context.

OdResult traverseSceneGraph(OdNwModelItemPtr pNode, RepoNwTraversalContext context, const bool inCompositeObject = false)
{
	auto levelName = convertToStdString(pNode->getDisplayName());

	// The OdNwPartition distinguishes between branches of the scene graph from
	// different files.
	// https://docs.opendesign.com/bimnv/OdNwPartition.html
	// OdNwModelItemPtr does not self-report this type in getIcon() as with other
	// types, so we test for it with a cast.

	auto pPartition = OdNwPartition::cast(pNode);
	if (!pPartition.isNull())
	{
		context.partition = pPartition;

		// When "changing file", remove the path from the name for the purposes of
		// building the tree.

		if (!levelName.empty())
		{
			levelName = boost::filesystem::path(levelName).filename().string();
		}
	}

	// Some items (e.g. certain IFC entries) don't have names, so we use their Type
	// instead to name the layer.

	if (levelName.empty())
	{
		levelName = convertToStdString(pNode->getClassDisplayName());
	}

	// Collect the metadata for this node; even if this node does not exist in our
	// tree we still need it to combine with the metadata of it's descendants (see
	// below).

	// It turns out to be far, far faster to call processAttributes on every
	// drawable and store the metadata, than to store a pointer to the parent and
	// get the metadata on-demand. (The mechanism is not clear but it is probably
	// something like ODA having to seek the file in order to open properties for
	// an element that is not next to pNode, etc.)

	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
	processAttributes(pNode, context, metadata);

	// The importer will not create tree entries for Instance nodes, though it will
	// import their metadata.

	const bool shouldImport = !isInstanced(pNode);
	bool isComposite = inCompositeObject || pNode->getIcon() == NwModelItemIcon::COMPOSITE_OBJECT;

	if (shouldImport)
	{
		// If we're building a composite object, Keep the layer the same.
		if (!inCompositeObject) {

			// Otherwise add a new layer for this node

			context.parentNode = context.sceneBuilder->addNode(RepoBSONFactory::makeTransformationNode({}, levelName, { context.parentNode->getSharedID() }));

			// GetIcon distinguishes the type of node. This corresponds to the icon seen in
			// the Selection Tree View in Navisworks.
			// https://docs.opendesign.com/bimnv/OdNwModelItem__getIcon@const.html
			// https://knowledge.autodesk.com/support/navisworks-products/learn-explore/caas/CloudHelp/cloudhelp/2017/ENU/Navisworks-Manage/files/GUID-BC657B3A-5104-45B7-93A9-C6F4A10ED0D4-htm.html
			// (Note "INSERT" -> "INSTANCED")

			if (pNode->getIcon() == NwModelItemIcon::LAYER)
			{
				context.layer = pNode;
			}

			// To match the plug-in, and ensure metadata ends up in the right place for
			// the benefit of smart groups, node properties are overridden with their
			// parent's metadata
			std::unordered_map<std::string, repo::lib::RepoVariant> merged = metadata;
			for (auto p : context.parentMetadata) {
				merged[p.first] = p.second;
			}

			context.sceneBuilder->addNode(RepoBSONFactory::makeMetaDataNode(merged, context.parentNode->getName(), { context.parentNode->getSharedID() }));
		}

		if (pNode->hasGeometry())
		{
			processGeometry(pNode, context); // Create meshes, parented to the current node in the context
		}
	}

	OdNwObjectIdArray aNodeChildren;
	OdResult res = pNode->getChildren(aNodeChildren);
	if (res != eOk)
	{
		return res;
	}

	// The node for getting parent metadata should always be the parent in the Nw
	// tree, even if its a (e.g. instance) node that doesn't appear in our tree.

	context.parentMetadata = metadata;

	for (OdNwObjectIdArray::const_iterator itRootChildren = aNodeChildren.begin(); itRootChildren != aNodeChildren.end(); ++itRootChildren)
	{
		if (!itRootChildren->isNull())
		{
			res = traverseSceneGraph(OdNwModelItem::cast(itRootChildren->safeOpenObject()), context, isComposite);
			if (res != eOk)
			{
				return res;
			}
		}
	}
	return eOk;
}

// Given a Partition (a file linkage, in an NWF), get the transform that converts
// from its local coordinate system to the 3DR Project Coordinate system.

OdGeMatrix3d getPartitionTransform(OdNwPartitionPtr pPartition)
{
	auto y = pPartition->getUpVector();
	if (y.isZeroLength()) {
		y = OdGeVector3d::kZAxis; // 0,0,1 and 0,1,0 are the defaults for Up and North in Navis.
	}

	auto z = pPartition->getNorthVector();
	if (z.isZeroLength()) {
		z = OdGeVector3d::kYAxis;
	}

	auto x = z.crossProduct(y);

	// As a reminder, the Project/3DR coordinate system conventions are:
	// +Y -> North
	// +X -> East
	// +Z -> Elevation

	return OdGeMatrix3d::alignCoordSys(
		OdGePoint3d::kOrigin,
		x,
		y,
		z,
		OdGePoint3d::kOrigin,
		OdGeVector3d::kXAxis,
		OdGeVector3d::kZAxis,
		OdGeVector3d::kYAxis
	);
}

void DataProcessorNwd::process(OdNwDatabasePtr pNwDb)
{
	repo::lib::RepoBounds bounds;

	OdNwObjectIdArray models;
	pNwDb->getModels(models);
	for (auto& model : models)
	{
		OdNwModelPtr pModel = model.safeOpenObject();
		OdNwModelItemPtr pModelItemRoot = pModel->getRootItem().safeOpenObject();

		// The world-offset is applied in processGeometry after conversion to project
		// coordinates, so the bounds must also be calculated in project coordinates

		auto b = pModelItemRoot->getBoundingBox();
		b.transformBy(getPartitionTransform(OdNwPartition::cast(pModelItemRoot)));
		bounds.encapsulate(toRepoBounds(b));
	}

	RepoNwTraversalContext context;
	context.sceneBuilder = this->builder;
	context.sceneBuilder->setWorldOffset(bounds.min());
	context.vectorizer = OdVectorizer::createObject(pNwDb);
	context.parentNode = context.sceneBuilder->addNode(RepoBSONFactory::makeTransformationNode({}, "rootNode"));

	for (auto& model : models)
	{
		OdNwModelPtr pModel = model.safeOpenObject();
		OdNwModelItemPtr pModelItemRoot = pModel->getRootItem().safeOpenObject();
		context.layer = pModelItemRoot;

		// Each linked file (Model) can have its own World Orientation and Units and
		// Transformations set. In practice though, when saving an NWD/NWC (the only
		// files we support), Navis introduces a root node for the new NWD itself,
		// and bakes the federation under it, so we only ever really have the World
		// Orientation to worry about.

		OdNwPartitionPtr pPartition = OdNwPartition::cast(pModelItemRoot);
		context.transform = getPartitionTransform(pPartition);

		traverseSceneGraph(pModelItemRoot, context);
	}
}
