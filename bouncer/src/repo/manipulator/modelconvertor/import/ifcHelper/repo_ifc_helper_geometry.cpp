/**
*  Copyright (C) 2015 3D Repo Ltd
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

/**
* Allows geometry creation using ifcopenshell
*/

#define NOMINMAX

#include <ifcpp/model/BuildingModel.h>
#include <ifcpp/reader/ReaderSTEP.h>
#include <ifcpp/geometry/Carve/GeometryConverter.h>

#include <locale>
#include <codecvt>

#include "repo_ifc_helper_geometry.h"
#include "../../../../core/model/bson/repo_bson_factory.h"
#include <boost/filesystem.hpp>
#include "../repo_model_import_config_default_values.h"

using namespace repo::manipulator::modelconvertor::ifcHelper;

const double eps = 1.5e-08;

IFCUtilsGeometry::IFCUtilsGeometry(const std::string &file, const modelConverter::ModelImportConfig &settings) :
	file(file)
{
}

IFCUtilsGeometry::~IFCUtilsGeometry()
{
}

std::wstring toWString(const std::string &str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(str);

	return wide;
}

std::string toNString(const std::wstring &str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::string narrow = converter.to_bytes(str);

	return narrow;
}

repo::core::model::MeshNode* processMesh(
	const carve::mesh::Mesh<3> * mesh,
	const carve::math::Matrix &matrix,
	double minTriArea,
	const vec3 &offsetVec
) {
	std::vector<repo_face_t> faces;
	std::vector<repo::lib::RepoVector3D> vertices;

	for (const auto &face : mesh->faces) {
		const size_t nVertices = face->nVertices();
		auto e = face->edge;
		if (face->n_edges == 3)
		{
			auto edge0 = face->edge;
			auto edge1 = edge0->next;
			auto edge2 = edge1->next;
			auto v0 = edge0->vert;
			auto v1 = edge1->vert;
			auto v2 = edge2->vert;

			vec3 vert0 = v0->v;
			vec3 vert1 = v1->v;
			vec3 vert2 = v2->v;

			vec3 v0v1 = vert1 - vert0;
			vec3 v0v2 = vert2 - vert0;
			double area = (carve::geom::cross(v0v1, v0v2).length())*0.5;
			if (abs(area) > minTriArea)   // skip degenerated triangle
			{
				vert0 = matrix * vert0 - offsetVec;
				vert1 = matrix * vert1 - offsetVec;
				vert2 = matrix * vert2 - offsetVec;
				auto firstVertex = (uint32_t)vertices.size();
				vertices.push_back({ (float)vert0.x, (float)vert0.y, (float)vert0.z });
				vertices.push_back({ (float)vert1.x, (float)vert1.y, (float)vert1.z });
				vertices.push_back({ (float)vert2.x, (float)vert2.y, (float)vert2.z });

				faces.push_back({ firstVertex, firstVertex + 1, firstVertex + 2 });
			}
		}
		else {
			repoError << "Untriangulated faces found (nEdges : " << face->n_edges << ").";
		}
	}

	auto bbox = mesh->getAABB();
	auto min = matrix * bbox.min() - offsetVec;
	auto max = matrix * bbox.max() - offsetVec;

	const std::vector<std::vector<float>> boundingBox = {
		{ (float)min.x, (float)min.y, (float)(min.z) },
		{ (float)max.x, (float)max.y, (float)(max.z) },
	};

	return vertices.size() ?
		new repo::core::model::MeshNode(repo::core::model::RepoBSONFactory::makeMeshNode(vertices, faces, std::vector<repo::lib::RepoVector3D>(), boundingBox))
		: nullptr;
}

std::vector<repo::core::model::MeshNode*> processShapeData(
	const std::shared_ptr<ProductShapeData> &shapeData,
	const double &minTriArea,
	const vec3 &offsetVec
) {
	std::vector<repo::core::model::MeshNode*> meshNodes;
	if (shapeData->m_ifc_object_definition.expired()) return meshNodes;

	std::shared_ptr<IfcObjectDefinition> objDef(shapeData->m_ifc_object_definition);

	auto ifcProductPtr = std::dynamic_pointer_cast<IfcProduct>(objDef);
	if (!ifcProductPtr ||
		std::dynamic_pointer_cast<IfcFeatureElementSubtraction>(ifcProductPtr) ||
		!ifcProductPtr->m_Representation) return meshNodes;

	for (const auto &rep : shapeData->m_vec_representations) {
		if (rep->m_ifc_representation.expired()) continue;

		std::shared_ptr<IfcRepresentation> ifcRep(rep->m_ifc_representation);

		for (const auto &items : rep->m_vec_item_data) {
			for (auto &meshOpen : items->m_meshsets_open) {
				CSG_Adapter::retriangulateMeshSet(meshOpen);
				for (const auto &mesh : meshOpen->meshes)
				{
					auto meshNode = processMesh(mesh, shapeData->getTransform(), minTriArea, offsetVec);
					if (meshNode) {
						meshNodes.push_back(meshNode);
					}
				}
			}

			for (auto &meshGroup : items->m_meshsets) {
				CSG_Adapter::retriangulateMeshSet(meshGroup);
				for (const auto &mesh : meshGroup->meshes)
				{
					auto meshNode = processMesh(mesh, shapeData->getTransform(), minTriArea, offsetVec);
					if (meshNode) {
						meshNodes.push_back(meshNode);
					}
				}
			}
		}
	}

	return meshNodes;
}

bool IFCUtilsGeometry::generateGeometry(
	std::string &errMsg,
	bool        &partialFailure)
{
	std::shared_ptr<BuildingModel> model = std::make_shared<BuildingModel>();
	std::shared_ptr<ReaderSTEP> stepReader = std::make_shared<ReaderSTEP>();

	stepReader->loadModelFromFile(toWString(file), model);
	//extract geometry
	std::shared_ptr<GeometryConverter> geometryConv = std::make_shared<GeometryConverter>(model);
	geometryConv->setCsgEps(eps* geometryConv->getBuildingModel()->getUnitConverter()->getLengthInMeterFactor());
	geometryConv->convertGeometry();

	auto geomSettings = geometryConv->getGeomSettings();
	auto shapeData = geometryConv->getShapeInputData();

	std::vector<repo::lib::RepoUUID> meshSharedIds;

	for (const auto &shape : shapeData)
	{
		auto shapePtr = shape.second;
		if (shapePtr) {
			carve::geom3d::AABB bbox;
			shapePtr->computeBoundingBox(bbox, true);

			auto min = shapePtr->getTransform() * bbox.min();
			if (offset.size()) {
				offset[0] = offset[0] < min.x ? offset[0] : min.x;
				offset[1] = offset[1] < min.y ? offset[1] : min.y;
				offset[2] = offset[2] < min.z ? offset[2] : min.z;
			}
			else {
				offset = { min.x, min.y , min.z };
			}
		}
	};

	vec3 offsetVec;
	if (offset.size()) {
		offsetVec.x = offset[0];
		offsetVec.y = offset[1];
		offsetVec.z = offset[2];
	}

	for (const auto &shape : shapeData)
	{
		if (shape.second) {
			auto productGeos = processShapeData(shape.second, geomSettings->getMinTriangleArea(), offsetVec);
			if (productGeos.size()) {
				std::shared_ptr<IfcObjectDefinition> objDef(shape.second->m_ifc_object_definition);
				meshes[toNString(objDef->m_GlobalId->m_value)] = productGeos;
				for (const auto &mesh : productGeos)
					meshSharedIds.push_back(mesh->getSharedID());
			}
		}
	}

	// create default mat
/*
	repo_material_t matProp;
	matProp.diffuse = { 0.5, 0.0, 0.0, 1 };

	auto matNode = repo::core::model::RepoBSONFactory::makeMaterialNode(matProp, "Default", meshSharedIds);
	materials["Default"] = new repo::core::model::MaterialNode(matNode);*/

	model->clearIfcModel();

	//std::vector<std::vector<repo_face_t>> allFaces;
	//std::vector<std::vector<double>> allVertices;
	//std::vector<std::vector<double>> allNormals;
	//std::vector<std::vector<double>> allUVs;
	//std::vector<std::string> allIds, allNames, allMaterials;
	//std::unordered_map<std::string, repo_material_t> matNameToMaterials;

	//switch (getIFCSchema(file)) {
	//case IfcSchemaVersion::IFC2x3:
	//	if (!repo::ifcUtility::Schema_Ifc2x3::GeometryHandler::retrieveGeometry(file,
	//		allVertices, allFaces, allNormals, allUVs, allIds, allNames, allMaterials, matNameToMaterials, offset, errMsg))
	//		return false;
	//	break;
	//case IfcSchemaVersion::IFC4:
	//	if (!repo::ifcUtility::Schema_Ifc4::GeometryHandler::retrieveGeometry(file,
	//		allVertices, allFaces, allNormals, allUVs, allIds, allNames, allMaterials, matNameToMaterials, offset, errMsg))
	//		return false;
	//	break;
	//default:
	//	errMsg = "Unsupported IFC Version";
	//	return false;
	//}

	////now we have found all meshes, take the minimum bounding box of the scene as offset
	////and create repo meshes
	//repoTrace << "Finished iterating. number of meshes found: " << allVertices.size();
	//repoTrace << "Finished iterating. number of materials found: " << materials.size();

	//std::map<std::string, std::vector<repo::lib::RepoUUID>> materialParent;
	//std::string defaultMaterialName = "_3DREPO_DEFAULT_MAT";
	//std::vector<repo::lib::RepoUUID> parentsOfDefault;
	//for (int i = 0; i < allVertices.size(); ++i)
	//{
	//	std::vector<repo::lib::RepoVector3D> vertices, normals;
	//	std::vector<repo::lib::RepoVector2D> uvs;
	//	std::vector<repo_face_t> faces;
	//	std::vector<std::vector<float>> boundingBox;
	//	for (int j = 0; j < allVertices[i].size(); j += 3)
	//	{
	//		vertices.push_back({ (float)(allVertices[i][j] - offset[0]), (float)(allVertices[i][j + 1] - offset[1]), (float)(allVertices[i][j + 2] - offset[2]) });
	//		if (allNormals[i].size())
	//			normals.push_back({ (float)allNormals[i][j], (float)allNormals[i][j + 1], (float)allNormals[i][j + 2] });

	//		auto vertex = vertices.back();
	//		if (j == 0)
	//		{
	//			boundingBox.push_back({ vertex.x, vertex.y, vertex.z });
	//			boundingBox.push_back({ vertex.x, vertex.y, vertex.z });
	//		}
	//		else
	//		{
	//			boundingBox[0][0] = boundingBox[0][0] > vertex.x ? vertex.x : boundingBox[0][0];
	//			boundingBox[0][1] = boundingBox[0][1] > vertex.y ? vertex.y : boundingBox[0][1];
	//			boundingBox[0][2] = boundingBox[0][2] > vertex.z ? vertex.z : boundingBox[0][2];

	//			boundingBox[1][0] = boundingBox[1][0] < vertex.x ? vertex.x : boundingBox[1][0];
	//			boundingBox[1][1] = boundingBox[1][1] < vertex.y ? vertex.y : boundingBox[1][1];
	//			boundingBox[1][2] = boundingBox[1][2] < vertex.z ? vertex.z : boundingBox[1][2];
	//		}
	//	}

	//	for (int j = 0; j < allUVs[i].size(); j += 2)
	//	{
	//		uvs.push_back({ (float)allUVs[i][j], (float)allUVs[i][j + 1] });
	//	}

	//	std::vector < std::vector<repo::lib::RepoVector2D>> uvChannels;
	//	if (uvs.size())
	//		uvChannels.push_back(uvs);

	//	auto mesh = repo::core::model::RepoBSONFactory::makeMeshNode(vertices, allFaces[i], normals, boundingBox, uvChannels,
	//		std::vector<repo_color4d_t>(), std::vector<std::vector<float>>());

	//	if (meshes.find(allIds[i]) == meshes.end())
	//	{
	//		meshes[allIds[i]] = std::vector<repo::core::model::MeshNode*>();
	//	}
	//	meshes[allIds[i]].push_back(new repo::core::model::MeshNode(mesh));

	//	if (allMaterials[i] != "")
	//	{
	//		if (materialParent.find(allMaterials[i]) == materialParent.end())
	//		{
	//			materialParent[allMaterials[i]] = std::vector<repo::lib::RepoUUID>();
	//		}

	//		materialParent[allMaterials[i]].push_back(mesh.getSharedID());
	//	}
	//	else
	//	{
	//		//This mesh has no material, assigning a default
	//		if (matNameToMaterials.find(defaultMaterialName) == matNameToMaterials.end())
	//		{
	//			repo_material_t matProp;
	//			matProp.diffuse = { 0.5, 0.5, 0.5, 1 };
	//			matNameToMaterials[defaultMaterialName] = matProp;
	//		}
	//		if (materialParent.find(defaultMaterialName) == materialParent.end())
	//		{
	//			materialParent[defaultMaterialName] = std::vector<repo::lib::RepoUUID>();
	//		}
	//		materialParent[defaultMaterialName].push_back(mesh.getSharedID());
	//	}
	//}

	//repoTrace << "Meshes constructed. Wiring materials to parents...";

	//for (const auto &pair : materialParent)
	//{
	//	auto matIt = matNameToMaterials.find(pair.first);
	//	if (matIt != matNameToMaterials.end())
	//	{
	//		auto matNode = (new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(matIt->second, pair.first, pair.second)));
	//		materials[pair.first] = matNode;
	//	}
	//}

	//return true;
}