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

// ODA
#include <OdaCommon.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <StaticRxObject.h>
#include <ExSystemServices.h>
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

// boost
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

using namespace repo::manipulator::modelconvertor::odaHelper;

static std::vector<std::string> ignoredCategories = { "Material", "Geometry", "Autodesk Material", "Revit Material" };
static std::vector<std::string> ignoredKeys = { "Item::Source File Name" };
static std::string sElementIdKey = "Element ID::Value";

struct RepoNwTraversalContext {
	OdNwModelItemPtr layer;
	OdNwPartitionPtr partition;
	OdNwModelItemPtr parent;
	std::string parentLayerId;
	GeometryCollector* collector;
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

template <class T>
void setMetadataValue(const OdString& category, const OdString& key, const T& value, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	auto metaKey = convertToStdString(category) + "::" + (key.isEmpty() ? std::string("Value") : convertToStdString(key));
	auto metaValue = value;
	metadata[metaKey] = metaValue;
}

// This specialisation is required for Linux as lexical cast doesn't know what to with OdString there
template <>
void setMetadataValue(const OdString& category, const OdString& key, const OdString& value, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	setMetadataValue(category, key, convertToStdString(value), metadata);
}

// This specialisation is because v140 doesn't support constexpr and will attempt to find a to_string overload regardless of the if-statement
template <>
void setMetadataValue(const OdString& category, const OdString& key, const double& value, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	setMetadataValue(category, key, value, metadata);
}

void removeFilepathFromMetadataValue(std::string key, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (metadata.find(key) != metadata.end()) {
		std::string strMetaData = "";
		if(metadata[key].getStringData(strMetaData)){
			metadata[key] = boost::filesystem::path(strMetaData).filename().string();
		}
	}
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
	auto ItemLayer = convertToStdString(context.layer->getDisplayName());

	if (!OdNwPartition::cast(context.layer).isNull())
	{
		ItemLayer = boost::filesystem::path(ItemLayer).filename().string();
	}

	auto ItemGuid = modelItemPtr->getInstanceGuid().toString();
	auto hasGuid = modelItemPtr->getInstanceGuid() != OdGUID::kNull;

	// Print the custom Item block to the metadata

	setMetadataValue(OD_T("Item"), OD_T("Name"), ItemName, metadata);
	setMetadataValue(OD_T("Item"), OD_T("Type"), ItemType, metadata);
	setMetadataValue(OD_T("Item"), OD_T("Internal Type"), ItemInternalType, metadata);
	setMetadataValue(OD_T("Item"), OD_T("Hidden"), ItemHidden, metadata);
	setMetadataValue(OD_T("Item"), OD_T("Layer"), ItemLayer, metadata);
	if (hasGuid) {
		setMetadataValue(OD_T("Item"), OD_T("GUID"), ItemGuid, metadata);
	}

	// The source file name is contained within the OdNwPartition entry in the
	// scene graph, which segments branches. Note that this will contain the
	// entire path (the path where the NWD is stored, if an NWD).

	if (!context.partition.isNull()) {
		setMetadataValue(OD_T("Item"), OD_T("Source File"), context.partition->getSourceFileName(), metadata);
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

		auto name = attribute->getClassName();
		auto category = attribute->getClassDisplayName();

		// Ignore some specific Categories based on their name, regardless of
		// Attribute Type

		if (std::find(ignoredCategories.begin(), ignoredCategories.end(), convertToStdString(category)) != ignoredCategories.end()) {
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
			OdArray<OdNwPropertyPtr> properties;
			propertiesAttribute->getProperties(properties);

			for (unsigned int j = 0; j < properties.length(); j++)
			{
				auto& prop = properties[j];
				auto key = prop->getDisplayName();

				bool bValue;
				double dValue;
				double fValue;
				OdInt32 i32Value;
				OdInt8 i8value;
				OdUInt32 u32value;
				OdUInt8 u8value;
				OdString sValue;
				OdUInt64 u64value;
				OdStringArray arrValue;
				OdGeVector3d vValue;
				OdNwColor cValue;
				tm tmValue;
				std::string tempValue;

				switch (prop->getValueType())
				{
				case NwPropertyValueType::value_type_default:
					break;
				case NwPropertyValueType::value_type_bool:
					prop->getValue(bValue);
					setMetadataValue(category, key, bValue, metadata);
					break;
				case NwPropertyValueType::value_type_double:
					prop->getValue(dValue);
					setMetadataValue(category, key, dValue, metadata);
					break;
				case NwPropertyValueType::value_type_float:
					prop->getValue(fValue);
					setMetadataValue(category, key, fValue, metadata);
					break;
				case NwPropertyValueType::value_type_OdInt32:
					prop->getValue(i32Value);
					setMetadataValue(category, key, i32Value, metadata);
					break;
				case NwPropertyValueType::value_type_OdInt8:
					prop->getValue(i8value);
					setMetadataValue(category, key, i8value, metadata);
					break;
				case NwPropertyValueType::value_type_OdUInt32:
					prop->getValue(u32value);
					tempValue = boost::lexical_cast<std::string>(u32value);
					setMetadataValue(category, key,  tempValue, metadata);
					break;
				case NwPropertyValueType::value_type_OdUInt8:
					prop->getValue(u8value);
					setMetadataValue(category, key, u8value, metadata);
					break;
				case NwPropertyValueType::value_type_OdString:
					prop->getValue(sValue);
					setMetadataValue(category, key, convertToStdString(sValue), metadata);
					break;
				case NwPropertyValueType::value_type_OdStringArray:
				{
					prop->getValue(arrValue);
					std::string combined;
					for (auto str : arrValue)
					{
						combined += convertToStdString(str) + ";";
					}
					setMetadataValue(category, key, combined, metadata);
				}
				break;
				case NwPropertyValueType::value_type_OdGeVector3d:
				{
					prop->getValue(vValue);
					auto value = boost::lexical_cast<std::string>(vValue.x) + ", " + boost::lexical_cast<std::string>(vValue.y) + ", " + boost::lexical_cast<std::string>(vValue.z);
					setMetadataValue(category, key, value, metadata);
				}
				break;
				case NwPropertyValueType::value_type_OdUInt64:
					prop->getValue(u64value);
					setMetadataValue(category, key, u64value, metadata);
					break;
				case NwPropertyValueType::value_type_OdNwColor:
				{
					prop->getValue(cValue);
					auto value = boost::lexical_cast<std::string>(cValue.R()) + ", " + boost::lexical_cast<std::string>(cValue.G()) + ", " + boost::lexical_cast<std::string>(cValue.B()) + ", " + boost::lexical_cast<std::string>(cValue.A());
					setMetadataValue(category, key, value, metadata);
				}
				break;
				case NwPropertyValueType::value_type_tm:
				{
					prop->getValue(tmValue);
					char buffer[80];
					std::strftime(buffer, 80, "%c", &tmValue);
					setMetadataValue(category, key, std::string(buffer), metadata);
				}
				break;
				}
			}
		}

		// Translation and Rotation attribute
		auto transRotationAttribute = OdNwTransRotationAttribute::cast(attribute);
		if (!transRotationAttribute.isNull())
		{
			auto translation = transRotationAttribute->getTranslation();
			auto rotation = transRotationAttribute->getRotation();

			setMetadataValue(OD_T("Transform"), OD_T("Rotation:X"), rotation.x, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Rotation:Y"), rotation.y, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Rotation:Z"), rotation.z, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Rotation:W"), rotation.w, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Translation:X"), translation.x, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Translation:Y"), translation.y, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Translation:Z"), translation.z, metadata);
		}

		// Transform matrix attribute
		auto transformAttribute = OdNwTransformAttribute::cast(attribute);
		if (!transformAttribute.isNull())
		{
			setMetadataValue(OD_T("Transform"), OD_T("Reverses"), transformAttribute->isReverse(), metadata);
			auto matrix = transformAttribute->getTransform();

			OdGeVector3d translation(matrix.entry[0][3], matrix.entry[1][3], matrix.entry[2][3]);
			setMetadataValue(OD_T("Transform"), OD_T("Translation:X"), translation.x, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Translation:Y"), translation.y, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Translation:Z"), translation.z, metadata);

			//Todo: extract rotation (in angle axis format) and scale
		}

		// Translation attribute
		auto transAttribute = OdNwTransAttribute::cast(attribute);
		if (!transAttribute.isNull())
		{
			auto translation = transAttribute->getTranslation();
			setMetadataValue(OD_T("Transform"), OD_T("Translation:X"), translation.x, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Translation:Y"), translation.y, metadata);
			setMetadataValue(OD_T("Transform"), OD_T("Translation:Z"), translation.z, metadata);
		}

		// (Note The Guid in the Item block is not an attribute, but rather is retrieved using getInstanceGuid(). This will be something else.)

		auto guidAttribute = OdNwGuidAttribute::cast(attribute);
		if (!guidAttribute.isNull())
		{
			setMetadataValue(category, guidAttribute->getDisplayName(), convertToStdString(guidAttribute->getGuid().toString()), metadata);
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
			setMetadataValue(OD_T("Item"), OD_T("Material"), convertToStdString(value), metadata);
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
			std::string combined;
			for (auto url : urls) {
				if (!url.isNull()) {
					combined += convertToStdString(url->getURL()) + ";";
				}
			}
			setMetadataValue(category, urlAttribute->getDisplayName(), combined, metadata);
		}

		auto textAttribute = OdNwTextAttribute::cast(attribute);
		if (!textAttribute.isNull())
		{
			setMetadataValue(category, textAttribute->getDisplayName(), convertToStdString(textAttribute->getText()), metadata);
		}

		//Other Attributes include:
		//	OdNwGraphicMaterial
		//	OdNwName
		//	OdNwPresenterMaterial
		//	OdNwPresenterTextureSpace
		//	OdNwPublish
		// These are currently ignored.
	}

	// Make any necessary adjustments to the metadata.
	// Here we remove the path from keys known to contain filenames.

	removeFilepathFromMetadataValue("Item::Source File", metadata);
	removeFilepathFromMetadataValue("Item::File Name", metadata);

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
	context.collector->setCurrentMaterial(repoMaterial);

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

						context.collector->addFace(vertices);
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

					context.collector->addFace(vertices, normal, uvs);
				}

				continue;
			}

			// The full set of types that the OdNwGeometry could be are described
			// here: https://docs.opendesign.com/bimnv/OdNwGeometry.html
		}
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

		// When "changing file", remove the path from the name fo the purposes of
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
			context.collector->setLayer(levelId, levelName, context.parentLayerId);
			context.parentLayerId = levelId; // Track the ancestry manually so we can skip nodes at will when building the output tree

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

			context.collector->setMetadata(levelId, metadata);
		}

		if (pNode->hasGeometry())
		{
			context.collector->setMeshGroup(inCompositeObject ? context.parentLayerId : levelId); // Group and Layer names must match, so the mesh is flagged as a partial object and different primitives are combined in the tree view
			context.collector->setNextMeshName(levelName);

			processGeometry(pNode, context);
		}

		context.collector->stopMeshEntry();
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
		context.collector = this->collector;
		context.layer = pModelItemRoot;
		traverseSceneGraph(pModelItemRoot, context);
	}
}