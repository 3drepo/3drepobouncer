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

#include "material_collector_rvt.h"

using namespace repo::manipulator::modelconvertor::odaHelper;

const float SHININESS_STRENGTH_DEFAULT = 0.5f;
const float SHININESS_DEFAULT = 0.25f;
const float OPACITY_DEFAULT = 1.0f;
const std::vector<float> SPECULAR_DEFAULT {0.5, 0.5, 0.5};

OdBmGeometryPtrArray MaterialCollectorRvt::getGeometriesFromNode(OdBmGNodePtr node)
{
	OdBmGeometryPtrArray geometries;

	OdBmGNodePtrArray nodes;
	if (node->isA() == OdBmGeometry::desc())
	{
		OdBmGeometryPtr pGeometry = (OdBmGeometry*)(node.get());

		OdBmFacePtrArray faces;
		pGeometry->getFaces(faces);

		if (!faces.isEmpty())
		{
			geometries.append(pGeometry);
			return geometries;
		}
	}
	else if (node->isA() == OdBmGFilter::desc())
	{
		OdBmGFilterPtr pFilter = (OdBmGFilter*)(node.get());
		pFilter->getAllSubNodes(nodes);
	}
	else if (node->isA() == OdBmGGroup::desc())
	{
		OdBmGGroupPtr pGroup = (OdBmGGroup*)(node.get());

		pGroup->getAllSubNodes(nodes);
	}
	else if (node->isA() == OdBmGInstance::desc())
	{
		OdBmGInstancePtr pInstance = (OdBmGInstance*)(node.get());

		//Geometry of FamilyInstance contains in FamilySymbol.
		OdBmInstanceInfoPtr pInstanceInfo = pInstance->getInstanceInfo();
		OdBmObjectId symbId = pInstanceInfo->getSymbolId();

		if (!symbId.isNull())
		{
			OdBmObjectPtr pSymbObj = symbId.openObject();

			if (!pSymbObj.isNull() && pSymbObj->isA() == OdBmFamilySymbol::desc())
			{
				OdBmFamilySymbolPtr pFamSymb = (OdBmFamilySymbol*)(pSymbObj.get());

				OdBmObjectPtr pGeomObj = pFamSymb->getGeometry();
				if (!pGeomObj.isNull() && pGeomObj->isA() == OdBmGElement::desc())
				{
					OdBmGElementPtr pGElem = (OdBmGElement*)(pGeomObj.get());
					pGElem->getAllSubNodes(nodes);
				}
			}
			else if (!pSymbObj.isNull() && pSymbObj->isA() == OdBmBaseRailingSym::desc())
			{
				OdBmBaseRailingSymPtr pRailingSym = (OdBmBaseRailingSym*)(pSymbObj.get());

				OdBmObjectPtr pGeomObj = pRailingSym->getGeometry();
				if (!pGeomObj.isNull() && pGeomObj->isA() == OdBmGElement::desc())
				{
					OdBmGElementPtr pGElem = (OdBmGElement*)(pGeomObj.get());
					pGElem->getAllSubNodes(nodes);
				}
			}
		}
	}

	for (size_t nodeIterator = 0; nodeIterator < nodes.size(); nodeIterator++)
	{
		OdBmGeometryPtrArray geoms = getGeometriesFromNode(nodes[nodeIterator]);
		if (!geoms.isEmpty())
		{
			geometries.append(geoms);
		}
	}

	return geometries;
}

inline bool MaterialCollectorRvt::isFileExist(const std::string& fileName)
{
	if (FILE *file = fopen(fileName.c_str(), "r"))
	{
		fclose(file);
		return true;
	}
	return false;
}

bool MaterialCollectorRvt::convertMaterialData(OdBmObjectId matId, repo_material_t* TMaterial)
{
	if ((TMaterial == nullptr) || (matId.isNull()))
		return false;

	OdDbHandle id = matId.getHandle();
	OdString str = id.ascii();
	OdBmObjectPtr obj = matId.openObject();

	if (obj.isNull() || (obj->isA() != OdBmMaterialElem::desc()))
		return false;

	OdBmMaterialElemPtr materialElem = dynamic_cast<OdBmMaterialElem*>(obj.get());

	OdBmMaterialPtr material = materialElem->getMaterial();
	OdBmCmColor color = material->getColor();
	double transparency = material->getTransparency();

	TMaterial->diffuse = { float(color.red()) / 255.0f, float(color.green()) / 255.0f, float(color.blue()) / 255.0f };
	TMaterial->ambient = TMaterial->diffuse;
	TMaterial->emissive = TMaterial->diffuse;
	TMaterial->shininessStrength = SHININESS_STRENGTH_DEFAULT;
	TMaterial->opacity = 1.f - transparency;
	TMaterial->specular = SPECULAR_DEFAULT;
	TMaterial->shininess = SHININESS_DEFAULT;
	TMaterial->isWireframe = true;
	TMaterial->isWireframe = true;

	return true;
}

int MaterialCollectorRvt::getGeometries(const OdGiDrawable* elemRec, OdBmGeometryPtrArray& geoms)
{
	if (elemRec != nullptr)
	{
		geoms.clear();
		int count = 0;
		const OdGiDrawable* pGeom = elemRec;

		if (pGeom->isA() == OdBmGElement::desc())
		{
			
			OdBmElementPtr pBody = elemRec;
			if (pBody->isA() == OdBmSysPanelFamSym::desc() ||
				pBody->isA() == OdBmFilledRegion::desc() ||
				pBody->isA() == OdBmRoomElem::desc() ||
				pBody->isA() == OdBmShaftOpening::desc() ||
				pBody->isA() == OdBmFamilySymbol::desc() ||
				pBody->isA() == OdBmBaseRailingSym::desc())
			{
				return 0;
			}
			if (pBody->isA() == OdBmBaseRailing::desc())
			{
				return 1;
			}

			if (pBody->isA() == OdBmFamilyInstance::desc())
			{
				OdBmFamilyInstancePtr instance = (OdBmFamilyInstance*)(pBody.get());
				OdBmInstanceInfoPtr instanceInfo = instance->getInstanceInfo();
				OdGeMatrix3d trfMatrix = instanceInfo->getTrf();
				if (trfMatrix == OdGeMatrix3d::kIdentity)
				{
					return 0;
				}

				OdBmObjectPtr pGeomObj = instance->getGeometry();
				if (!pGeomObj.isNull() && pGeomObj->isA() == OdBmGElement::desc())
				{
					OdBmGElementPtr pGElem = (OdBmGElement*)(pGeomObj.get());

					OdBmGNodePtrArray nodes;
					pGElem->getAllSubNodes(nodes);
					for (OdUInt32 nodesIterator = 0; nodesIterator < nodes.size(); nodesIterator++)
					{
						OdBmGeometryPtrArray geometries = getGeometriesFromNode(nodes[nodesIterator]);
						if (!geometries.isEmpty())
						{
							count += geometries.size();
							geoms.append(geometries);
						}
					}
				}
			}
			else
			{
				const OdBmGElement* pGElem = (const OdBmGElement*)(pGeom);
				OdBmGNodePtrArray nodes;
				pGElem->getAllSubNodes(nodes);
				for (OdUInt32 iIdx = 0; iIdx < nodes.size(); iIdx++)
				{
					if (nodes[iIdx]->isA() == OdBmGeometry::desc())
					{
						OdBmGeometry* geom = (OdBmGeometry*)(nodes[iIdx].get());
						OdBmFacePtrArray faces;
						geom->getFaces(faces);
						if (faces.size() > 0)
						{
							count++;
							geoms.append(geom);
						}
					}
				}
			}
		}

		return count;
	}

	return 0;
}

bool MaterialCollectorRvt::getMaterial(const OdGiDrawable* pDrawable, repo_material_t& mat)
{
	OdBmGeometryPtrArray geoms;
	auto id = (OdUInt64)(OdIntPtr)pDrawable->id();
	if (id && getGeometries(pDrawable, geoms) <= 0) return false;
	OdBmObjectIdArray facesMaterialId = getFacesMaterialId(pDrawable);
	repo_material_t material;
	
	for (int i = 0; i < facesMaterialId.size(); ++i)
	{
		if (convertMaterialData(facesMaterialId[i], &material))
		{
			mat = material;
			return true;
		}
	}
	return false;
}

OdBmObjectIdArray MaterialCollectorRvt::getFacesMaterialId(const OdGiDrawable* elemRec)
{
	OdBmObjectIdArray resultingIds;
	const OdBmObject* geom = static_cast<const OdBmObject*>(elemRec);

	if (geom->isA() == OdBmGElement::desc())
	{
		const OdBmGElement* pGElem = (const OdBmGElement*)(geom);
		OdBmGNodePtrArray nodes;
		pGElem->getAllSubNodes(nodes);

		for (OdUInt32 nodesIterator = 0; nodesIterator < nodes.size(); nodesIterator++)
		{
			OdBmGeometryPtrArray geometries = getGeometriesFromNode(nodes[nodesIterator]);

			for (OdUInt32 geomIterator = 0; geomIterator < geometries.size(); geomIterator++)
			{
				geometries[geomIterator]->getFaces(faces);
				for (OdUInt32 facesIterator = 0; facesIterator < faces.size(); facesIterator++)
				{
					resultingIds.push_back(faces[facesIterator]->getRenderStyleId());
				}
			}
		}
	}

	return resultingIds;
}

