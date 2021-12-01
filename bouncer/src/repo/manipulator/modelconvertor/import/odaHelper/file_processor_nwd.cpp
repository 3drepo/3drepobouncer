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

//ODA
#include <OdaCommon.h>
#include <RxDynamicModule.h>
#include <DynamicLinker.h>
#include <StaticRxObject.h>
#include <ExSystemServices.h>
#include <NwHostAppServices.h>
#include <NwDatabase.h>
#include <toString.h>


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
#include <NwTextFontInfo.h>
#include <NwModel.h>
#include <NwProperty.h>
#include <NwPartition.h>
#include <NwAttribute.h>

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

//3d repo bouncer
#include "file_processor_nwd.h"
#include "helper_functions.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

static OdString sNwDbModuleName = L"TNW_Db";

class OdExNwSystemServices : public ExSystemServices
{
public:
	OdExNwSystemServices() {}
};

class RepoNwServices : public OdExNwSystemServices, public OdNwHostAppServices
{
protected:
	ODRX_USING_HEAP_OPERATORS(OdExNwSystemServices);
};

repo::manipulator::modelconvertor::odaHelper::FileProcessorNwd::~FileProcessorNwd()
{
}

struct RepoNwTraversalContext {
	OdNwModelItemPtr layer;
	OdNwPartitionPtr partition;
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
	dest.push_back(color.R());
	dest.push_back(color.G());
	dest.push_back(color.B());
	dest.push_back(color.A());
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

	repo_material_t repoMaterial;



	OdNwObjectId materialId = pComp->getMaterialId();
	if (!materialId.isNull())
	{
		OdNwMaterialPtr pMaterial = materialId.safeOpenObject();
		OdNwColor odColor;

		if (!pMaterial->getDisplayName().compare("Site - Grass"))
		{
			std::cout << pMaterial->getDisplayName().c_str();
		}

		//std::cout << pMaterial->getDisplayName().c_str() << "\n";

		pMaterial->getDiffuse(odColor);
		convertColor(odColor, repoMaterial.diffuse);

		pMaterial->getAmbient(odColor);
		convertColor(odColor, repoMaterial.ambient);

		pMaterial->getEmissive(odColor);
		convertColor(odColor, repoMaterial.emissive);

		pMaterial->getSpecular(odColor);
		convertColor(odColor, repoMaterial.specular);

		repoMaterial.shininess = pMaterial->getShininess();
		repoMaterial.opacity = 1 - pMaterial->getTransparency();

		//if material has type of texture then can get texture's data
		if (pMaterial->isA() == OdNwTexture::desc())
		{
			//can get info about texture mappers
			OdNwTexturePtr pTextureMaterial = pMaterial;
			if (pTextureMaterial.isNull())
			{


			}
		}

		//Todo; finish materials
	}

	context.collector->setCurrentMaterial(repoMaterial);

	OdNwObjectIdArray aCompFragIds;
	pComp->getFragments(aCompFragIds);
	for (OdNwObjectIdArray::const_iterator itFrag = aCompFragIds.begin(); itFrag != aCompFragIds.end(); ++itFrag)
	{
		if (!(*itFrag).isNull())
		{
			OdNwFragmentPtr pFrag = OdNwFragment::cast((*itFrag).safeOpenObject());
			OdGeMatrix3d transformMatrix = pFrag->getTransformation();

			if (pFrag->isLineSet())
			{
				OdNwGeometryLineSetPtr pGeomertyLineSet = OdNwGeometryLineSet::cast(pFrag->getGeometryId().safeOpenObject());
				if (!pGeomertyLineSet.isNull())
				{
					// BimNv lines also have colours, but these are not supported yet.
					OdArray<OdGePoint3d> aVertexes = pGeomertyLineSet->getVertexes();
					OdArray<OdUInt16> aVertexPerLine = pGeomertyLineSet->getVertexCountPerLine();
					
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
				}
			}
			else if (pFrag->isMesh())
			{
				OdNwGeometryMeshPtr pGeometrMesh = OdNwGeometryMesh::cast(pFrag->getGeometryId().safeOpenObject());
				if (!pGeometrMesh.isNull())
				{
					const OdArray<OdGePoint3d>& aVertices = pGeometrMesh->getVertexes();
					const OdArray<OdGeVector3d>& aNormals = pGeometrMesh->getNormales();
					const OdArray<OdGePoint2d>& aUvs = pGeometrMesh->getUVParameters();

					const OdArray<OdNwTriangleIndexes>& aTriangles = pGeometrMesh->getTriangles();

					if (pGeometrMesh->getIndexes().length() && !aTriangles.length())
					{
						repoError << "Mesh " << pNode->getDisplayName().c_str() << " has indices but does not have a triangulation. Only triangulated meshes are supported.";
					}

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
				}
			}
			else if (pFrag->isPointSet())
			{
				//Todo; warning unsupported geometry type
			}
			else if (pFrag->isText())
			{
				//Todo; warning unsupported geometry type
			}
			else if (pFrag->isTube())
			{
				//Todo; warning unsupported geometry type
			}
			else if (pFrag->isEllipse())
			{
				// Todo; warning unsupported geometry type
			}
		}
	}
}

// Traverses the scene graph recursively. Each invocation of this method operates 
// on one OdNwModelItem, which is the basic OdNwObject that makes up the geometry 
// scene/the tree in the Standard View.
OdResult TraverseSceneGraph(OdNwModelItemPtr pNode, RepoNwTraversalContext context) 
{
	// The OdNwPartition distinguishes between branches of the scene graph from 
	// different files.
	// https://docs.opendesign.com/bimnv/OdNwPartition.html
	// OdNwModelItemPtr does not self-report this type in getIcon() as with other 
	// types, so we test for it with a cast.
	
	auto pPartition = OdNwPartition::cast(pNode);
	if (!pPartition.isNull())
	{
		context.partition = pPartition;
	}

	// GetIcon distinguishes the type of node. This corresponds to the icon seen in
	// the Selection Tree View in Navisworks.
	// https://docs.opendesign.com/bimnv/OdNwModelItem__getIcon@const.html
	// https://knowledge.autodesk.com/support/navisworks-products/learn-explore/caas/CloudHelp/cloudhelp/2017/ENU/Navisworks-Manage/files/GUID-BC657B3A-5104-45B7-93A9-C6F4A10ED0D4-htm.html
	// (Note "INSERT" -> "INSTANCED")

	// GeometryCollector::setLayer() is used to build the hierarchy. Each layer 
	// corresponds to a 'node' in the Navisworks Standard View and has a unique Id.
	// Calling setLayer will immediately create the layer, even before geometry is
	// added.

	// All items in the view are created as Transform nodes in the hierarchy on the
	// web, except Composite Objects, for which all child meshes should be collapsed.
	// The BimNv Object handles to re-construct the hierarchy.

	const auto levelName = convertToStdString(pNode->getDisplayName());
	const auto levelId = convertToStdString(toString(pNode->objectId().getHandle()));
	const auto levelParentId = pNode->getParent().isNull() ? std::string() : convertToStdString(toString(pNode->getParentId().getHandle()));

	context.collector->setLayer(levelId, levelName, levelParentId);

	if (pNode->getIcon() == NwModelItemIcon::COMPOSITE_OBJECT)
	{
		//ProcessAttributes(pNode, context);
	}
	
	if (pNode->getIcon() == NwModelItemIcon::LAYER)
	{
		context.layer = pNode;
	}
	
	if (pNode->getIcon() == NwModelItemIcon::INSERT_GEOMETRY && pNode->getIcon() != NwModelItemIcon::COMPOSITE_OBJECT)
	{
		//ProcessAttributes(pNode, context);
	}
	
	if (pNode->getIcon() == NwModelItemIcon::GEOMETRY && pNode->getIcon() != NwModelItemIcon::COMPOSITE_OBJECT)
	{
		//ProcessAttributes(pNode, context);
	}

	OdArray<OdNwAttributePtr> attributes;
	modelItemPtr->getAttributes(attributes, 0);



	if (pNode->hasGeometry())
	{
		context.collector->setNextMeshName(convertToStdString(pNode->getDisplayName()));
		processGeometry(pNode, context);
	}

	context.collector->stopMeshEntry();

	OdNwObjectIdArray aNodeChildren;
	OdResult res = pNode->getChildren(aNodeChildren);
	if (res != eOk)
	{
		return res;
	}

	for (OdNwObjectIdArray::const_iterator itRootChildren = aNodeChildren.begin(); itRootChildren != aNodeChildren.end(); ++itRootChildren)
	{
		if (!itRootChildren->isNull())
		{
			res = TraverseSceneGraph(OdNwModelItem::cast(itRootChildren->safeOpenObject()), context);
			if (res != eOk)
			{
				return res;
			}
		}
	}
	return eOk;
}

uint8_t FileProcessorNwd::readFile()
{
	int nRes = REPOERR_OK;

	OdStaticRxObject<RepoNwServices> svcs;
	odrxInitialize(&svcs);
	OdRxModule* pModule = ::odrxDynamicLinker()->loadModule(sNwDbModuleName, false);
	try
	{
		OdNwDatabasePtr pNwDb = svcs.readFile(OdString(file.c_str()));

		if (pNwDb.isNull())
		{
			throw new OdError("Failed to open file");
		}

		if (pNwDb->isComposite())
		{
			throw new OdError("Navisworks Composite/Index files (.nwf) are not supported. Files must contain embedded geometry (.nwd)");
		}

		OdNwObjectId modelItemRootId = pNwDb->getModelItemRootId();
		if (!modelItemRootId.isNull())
		{
			OdNwModelItemPtr pModelItemRoot = OdNwModelItem::cast(modelItemRootId.safeOpenObject());
			RepoNwTraversalContext context;
			context.collector = this->collector;
			TraverseSceneGraph(pModelItemRoot, context);
		}
	}
	catch (OdError& e)
	{
		repoError << convertToStdString(e.description()) << ", code: " << e.code();
		if (e.code() == OdResult::eUnsupportedFileFormat) {
			nRes = REPOERR_UNSUPPORTED_VERSION;
		}
		else {
			nRes = REPOERR_LOAD_SCENE_FAIL;
		}
	}
	odrxUninitialize();

	return nRes;
}