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
#include <NwGeometryEllipticalShape.h>
#include <NwGeometryLineSet.h>
#include <NwGeometryMesh.h>
#include <NwGeometryPointSet.h>
#include <NwGeometryText.h>
#include <NwGeometryTube.h>
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

// boost
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace repo::manipulator::modelconvertor::odaHelper;
using namespace repo::core::model;

static std::vector<std::string> ignoredCategories = { "Material", "Geometry", "Autodesk Material", "Revit Material" };
static std::vector<std::string> ignoredKeys = { "Item::Source File Name" };
static std::vector<std::string> keysForPathSanitation = { "Item::Source File", "Item::File Name" };
static std::string sElementIdKey = "Element ID::Value";

struct RepoNwTraversalContext {
	OdNwModelItemPtr layer;
	OdNwPartitionPtr partition;
	OdNwModelItemPtr parent;
	std::string parentLayerId;
	repo::manipulator::modelutility::RepoSceneBuilder* sceneBuilder;
	std::shared_ptr<repo::core::model::TransformationNode> parentNode;
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

void convertColor(OdNwColor color, std::vector<float>& dest)
{
	dest.clear();
	dest.push_back(color.R());
	dest.push_back(color.G());
	dest.push_back(color.B());
	dest.push_back(color.A());
}

void convertColor(OdString color, std::vector<float>& dest)
{
	dest.clear();
	try
	{
		typedef boost::tokenizer<boost::char_separator<OdChar>> tokenizer;
		boost::char_separator<OdChar> sep(OD_T(","));
		auto stringified = convertToStdString(color);
		tokenizer tokens(stringified, sep);
		for (tokenizer::iterator iter = tokens.begin(); iter != tokens.end(); iter++) {
			auto token = *iter;
			boost::trim(token);
			auto component = boost::lexical_cast<float>(token);
			dest.push_back(component / 255.0);
		}
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

	RepoMeshBuilder meshBuilder({ context.parentNode->getSharedID() });
	OdNwObjectIdArray aCompFragIds;
	pComp->getFragments(aCompFragIds);
	for (OdNwObjectIdArray::const_iterator itFrag = aCompFragIds.begin(); itFrag != aCompFragIds.end(); ++itFrag)
	{
		if (!(*itFrag).isNull())
		{
			OdNwFragmentPtr pFrag = OdNwFragment::cast((*itFrag).safeOpenObject());
			OdGeMatrix3d transformMatrix = pFrag->getTransformation();
			OdNwObjectId geometryId = pFrag->getGeometryId(); // Returns an object ID of the base class OdNwGeometry object with geometry data.

			if (geometryId.isNull() || !geometryId.isValid())
			{
				continue;
			}

			OdNwObjectPtr pGeometry = geometryId.safeOpenObject();

			// If the fragment has geometry, see if it is of a type that we support

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
				std::vector<repo::lib::RepoVector3D64> vertices;
				for (auto line = aVertexPerLine.begin(); line != aVertexPerLine.end(); line++)
				{
					for (auto i = 0; i < (*line - 1); i++)
					{
						vertices.clear();
						vertices.push_back(convertPoint(aVertexes[index + 0], transformMatrix));
						vertices.push_back(convertPoint(aVertexes[index + 1], transformMatrix));
						index++;

						meshBuilder.addFace(vertices);
					}

					index++;
				}

				continue;
			}

			OdNwGeometryMeshPtr pGeometryMesh = OdNwGeometryMesh::cast(pFrag->getGeometryId().safeOpenObject());
			if (!pGeometryMesh.isNull())
			{
				const OdArray<OdNwTriangleIndexes>& aTriangles = pGeometryMesh->getTriangles();

				if (pGeometryMesh->getIndices().length() && !aTriangles.length())
				{
					repoError << "Mesh " << pNode->getDisplayName().c_str() << " has indices but does not have a triangulation. Only triangulated meshes are supported.";
				}

				const OdNwVerticesDataPtr aVertexesData = pGeometryMesh->getVerticesData();

				const OdGePoint3dArray& aVertices = aVertexesData->getVertices();
				const OdGeVector3dArray& aNormals = aVertexesData->getNormals();
				const OdGePoint2dArray& aUvs = aVertexesData->getTexCoords();

				for (auto triangle = aTriangles.begin(); triangle != aTriangles.end(); triangle++)
				{
					std::vector<repo::lib::RepoVector3D64> vertices;
					vertices.push_back(convertPoint(aVertices[triangle->pointIndex1], transformMatrix));
					vertices.push_back(convertPoint(aVertices[triangle->pointIndex2], transformMatrix));
					vertices.push_back(convertPoint(aVertices[triangle->pointIndex3], transformMatrix));

					auto normal = calcNormal(vertices[0], vertices[1], vertices[2]);

					std::vector<repo::lib::RepoVector2D> uvs;
					if (aUvs.length())
					{
						uvs.push_back(convertPoint(aUvs[triangle->pointIndex1]));
						uvs.push_back(convertPoint(aUvs[triangle->pointIndex2]));
						uvs.push_back(convertPoint(aUvs[triangle->pointIndex3]));
					}

					meshBuilder.addFace(vertices, normal, uvs);
				}

				continue;
			}

			// The full set of types that the OdNwGeometry could be are described
			// here: https://docs.opendesign.com/bimnv/OdNwGeometry.html
		}
	}

	std::vector<repo::core::model::MeshNode> nodes;
	meshBuilder.extractMeshes(nodes);

	auto material = context.sceneBuilder->addNode(repo::core::model::RepoBSONFactory::makeMaterialNode(repoMaterial));
	for (auto& mesh : nodes)
	{
		material->addParent(mesh.getSharedID());
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
	// GeometryCollector::setLayer() is used to build the hierarchy. Each layer
	// corresponds to a 'node' in the Navisworks Standard View and has a unique Id.
	// Calling setLayer will immediately create the layer, even before geometry is
	// added.

	auto levelName = convertToStdString(pNode->getDisplayName());
	auto levelId = convertToStdString(toString(pNode->objectId().getHandle()));

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

			std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
			processAttributes(pNode, context, metadata);
			processAttributes(context.parent, context, metadata);

			context.sceneBuilder->addNode(RepoBSONFactory::makeMetaDataNode(metadata, context.parentNode->getName(), { context.parentNode->getSharedID() }));
		}

		if (pNode->hasGeometry())
		{
			// Create meshes, parented to the current node in the context

			processGeometry(pNode, context);
		}
	}

	OdNwObjectIdArray aNodeChildren;
	OdResult res = pNode->getChildren(aNodeChildren);
	if (res != eOk)
	{
		return res;
	}

	context.parent = pNode; // The node for getting parent metadata should always be the parent in the Nw tree, even if its a (e.g. instance) node that doesn't appear in our tree

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

void DataProcessorNwd::process(OdNwDatabasePtr pNwDb)
{
	OdNwObjectId modelItemRootId = pNwDb->getModelItemRootId();
	if (!modelItemRootId.isNull())
	{
		OdNwModelItemPtr pModelItemRoot = OdNwModelItem::cast(modelItemRootId.safeOpenObject());
		RepoNwTraversalContext context;
		context.sceneBuilder = this->builder;
		context.layer = pModelItemRoot;
		context.parentNode = context.sceneBuilder->addNode(RepoBSONFactory::makeTransformationNode({}, "rootNode"));
		traverseSceneGraph(pModelItemRoot, context);
	}
}
