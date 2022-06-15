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
#include "../../../../lib/datastructure/vertex_map.h"
#include <boost/filesystem.hpp>
#include "../repo_model_import_config_default_values.h"

using namespace repo::manipulator::modelconvertor::ifcHelper;

const double eps = 1.5e-8;

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

repo::core::model::MeshNode* processPolyLine(
	const std::shared_ptr<carve::input::PolylineSetData> &line,
	const carve::math::Matrix &matrix,
	const vec3 &offsetVec
) {
	if (line && line->points.size() > 1 && line->polylines.size()) {
		std::vector<float> min, max;
		auto polyLineSet = line->create(carve::input::opts());
		repo::lib::VertexMap32 vertexMap;
		std::vector<repo_face_t> faces;
		for (const auto &line : polyLineSet->lines) {
			int64_t lastIndx = -1;
			for (size_t i = 0; i < line->vertexCount(); ++i) {
				if (i <= polyLineSet->vertices.size()) {
					auto v = matrix * line->vertex(i)->v - offsetVec;
					auto vertex = repo::lib::RepoVector3D(
						(float)(v.x),
						(float)(v.y),
						(float)(v.z)
					);
					auto res = vertexMap.find(vertex);
					if (lastIndx >= 0) {
						repo_face_t face = { (uint32_t)lastIndx , res.index };
						faces.push_back(face);

						if (min.size()) {
							min[0] = min[0] < vertex.x ? min[0] : vertex.x;
							min[1] = min[1] < vertex.y ? min[1] : vertex.y;
							min[2] = min[2] < vertex.z ? min[2] : vertex.z;

							max[0] = max[0] < vertex.x ? max[0] : vertex.x;
							max[1] = max[1] < vertex.y ? max[1] : vertex.y;
							max[2] = max[2] < vertex.z ? max[2] : vertex.z;
						}
						else {
							min = { vertex.x, vertex.y, vertex.z };
							max = { vertex.x, vertex.y, vertex.z };
						}
					}

					lastIndx = res.index;
				}
			}
		}

		if (vertexMap.vertices.size()) {
			auto meshNode = new repo::core::model::MeshNode(repo::core::model::RepoBSONFactory::makeMeshNode(vertexMap.vertices, faces, vertexMap.normals, { min, max }));
			return meshNode;
		}

		return nullptr;
	}
}

repo::core::model::MeshNode* processMesh(
	const carve::mesh::Mesh<3> * mesh,
	const carve::math::Matrix &matrix,
	double minTriArea,
	const vec3 &offsetVec
) {
	std::vector<repo_face_t> faces;
	repo::lib::VertexMap32 vertexMap;

	for (const auto &face : mesh->faces) {
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

				repo::lib::RepoVector3D repoVec0 = { (float)vert0.x, (float)vert0.y, (float)vert0.z };
				repo::lib::RepoVector3D repoVec1 = { (float)vert1.x, (float)vert1.y, (float)vert1.z };
				repo::lib::RepoVector3D repoVec2 = { (float)vert2.x, (float)vert2.y, (float)vert2.z };

				repo::lib::RepoVector3D normal = (repoVec1 - repoVec0).crossProduct(repoVec2 - repoVec1);
				normal.normalize();

				repo_face_t face;
				auto res0 = vertexMap.find(repoVec0, normal);
				auto res1 = vertexMap.find(repoVec1, normal);
				auto res2 = vertexMap.find(repoVec2, normal);

				faces.push_back({ res0.index, res1.index, res2.index });
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

	return vertexMap.vertices.size() ?
		new repo::core::model::MeshNode(repo::core::model::RepoBSONFactory::makeMeshNode(vertexMap.vertices, faces, vertexMap.normals, boundingBox))
		: nullptr;
}

bool appearanceApplicable(const std::shared_ptr<AppearanceData> &ad) {
	return ad && (ad->m_apply_to_geometry_type == AppearanceData::GEOM_TYPE_SURFACE || ad->m_apply_to_geometry_type == AppearanceData::GEOM_TYPE_ANY);
}

repo::core::model::MaterialNode* createMatNode(const std::shared_ptr<AppearanceData> &data) {
	repo_material_t matAttrib;
	matAttrib.diffuse = { (float)data->m_color_diffuse.r(), (float)data->m_color_diffuse.g(), (float)data->m_color_diffuse.b(), (float)data->m_color_diffuse.a() };
	matAttrib.ambient = { (float)data->m_color_ambient.r(),(float)data->m_color_ambient.g(), (float)data->m_color_ambient.b(), (float)data->m_color_ambient.a() };
	matAttrib.specular = { (float)data->m_color_specular.r(), (float)data->m_color_specular.g(), (float)data->m_color_specular.b(), (float)data->m_color_specular.a() };

	matAttrib.shininess = data->m_shininess;
	matAttrib.opacity = data->m_transparency;

	return new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(matAttrib));
}

std::vector<repo::core::model::MeshNode*> processShapeData(
	const std::shared_ptr<ProductShapeData> &shapeData,
	const double &minTriArea,
	const vec3 &offsetVec,
	std::unordered_map<int, repo::core::model::MaterialNode*> &stepIdToMat,
	std::unordered_map<int, std::vector<repo::lib::RepoUUID>> &matToMeshes
) {
	std::vector<repo::core::model::MeshNode*> meshNodes;
	if (shapeData->m_ifc_object_definition.expired()) return meshNodes;

	std::shared_ptr<IfcObjectDefinition> objDef(shapeData->m_ifc_object_definition);

	auto ifcProductPtr = std::dynamic_pointer_cast<IfcProduct>(objDef);
	if (!ifcProductPtr ||
		std::dynamic_pointer_cast<IfcFeatureElementSubtraction>(ifcProductPtr) ||
		!ifcProductPtr->m_Representation) return meshNodes;

	std::shared_ptr<AppearanceData> shapeDataMat;

	for (const auto & appearance : shapeData->m_vec_product_appearances) {
		if (appearanceApplicable(appearance)) {
			shapeDataMat = appearance;
		}
	}

	for (const auto &rep : shapeData->m_vec_representations) {
		if (rep->m_ifc_representation.expired()) continue;

		std::shared_ptr<AppearanceData> repMat = shapeDataMat;

		if (!repMat) {
			std::shared_ptr<IfcRepresentation> ifcRep(rep->m_ifc_representation);
			for (const auto & appearance : rep->m_vec_representation_appearances) {
				if (appearanceApplicable(appearance)) {
					repMat = appearance;
				}
			}
		}

		for (const auto &items : rep->m_vec_item_data) {
			std::shared_ptr<AppearanceData> matToUse = repMat;
			if (!matToUse) {
				for (const auto & appearance : items->m_vec_item_appearances) {
					if (appearance) {
						if (appearanceApplicable(appearance)) {
							matToUse = appearance;
						}
					}
				}
			}

			int matId = -1;
			int lineMat = -2;
			if (matToUse) {
				matId = matToUse->m_step_style_id;
				if (stepIdToMat.find(matId) == stepIdToMat.end()) {
					stepIdToMat[matId] = createMatNode(matToUse);
				}
			}
			else {
				if (stepIdToMat.find(matId) == stepIdToMat.end()) {
					repo_material_t matProp;
					matProp.diffuse = { 0.5, 0.5, 0.5, 1 };

					auto matNode = repo::core::model::RepoBSONFactory::makeMaterialNode(matProp, "DefaultMat");
					stepIdToMat[matId] = new repo::core::model::MaterialNode(matNode);
				}
			}
			for (auto &meshOpen : items->m_meshsets_open) {
				CSG_Adapter::retriangulateMeshSet(meshOpen);
				for (const auto &mesh : meshOpen->meshes)
				{
					auto meshNode = processMesh(mesh, shapeData->getTransform(), minTriArea, offsetVec);
					if (meshNode) {
						meshNodes.push_back(meshNode);
						if (matToMeshes.find(matId) == matToMeshes.end()) {
							matToMeshes[matId] = { meshNode->getSharedID() };
						}
						else {
							matToMeshes[matId].push_back(meshNode->getSharedID());
						}
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
						if (matToMeshes.find(matId) == matToMeshes.end()) {
							matToMeshes[matId] = { meshNode->getSharedID() };
						}
						else {
							matToMeshes[matId].push_back(meshNode->getSharedID());
						}
					}
				}
			}

			for (const auto &line : items->m_polylines) {
				auto meshNode = processPolyLine(line, shapeData->getTransform(), offsetVec);
				if (meshNode) {
					meshNodes.push_back(meshNode);
					if (matToMeshes.find(lineMat) == matToMeshes.end()) {
						matToMeshes[lineMat] = { meshNode->getSharedID() };
					}
					else {
						matToMeshes[lineMat].push_back(meshNode->getSharedID());
					}

					if (stepIdToMat.find(lineMat) == stepIdToMat.end()) {
						repo_material_t matProp;
						matProp.diffuse = { 0,0,0, 1 };

						auto matNode = repo::core::model::RepoBSONFactory::makeMaterialNode(matProp, "DefaultPolyLineMat");
						stepIdToMat[lineMat] = new repo::core::model::MaterialNode(matNode);
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
	repoInfo << " !!!" << geometryConv->getBuildingModel()->getUnitConverter()->getLengthInMeterFactor();
	geometryConv->setCsgEps(eps* geometryConv->getBuildingModel()->getUnitConverter()->getLengthInMeterFactor());
	geometryConv->convertGeometry();

	auto geomSettings = geometryConv->getGeomSettings();
	auto shapeData = geometryConv->getShapeInputData();

	std::vector<repo::lib::RepoUUID> meshSharedIds;

	std::unordered_map<int, repo::core::model::MaterialNode*> stepIdToMat;
	std::unordered_map<int, std::vector<repo::lib::RepoUUID>> matToMeshes;

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
			auto productGeos = processShapeData(shape.second, geomSettings->getMinTriangleArea(), offsetVec, stepIdToMat, matToMeshes);
			if (productGeos.size()) {
				std::shared_ptr<IfcObjectDefinition> objDef(shape.second->m_ifc_object_definition);
				meshes[toNString(objDef->m_GlobalId->m_value)] = productGeos;
				for (const auto &mesh : productGeos)
					meshSharedIds.push_back(mesh->getSharedID());
			}
		}
	}

	for (const auto &entry : matToMeshes) {
		auto matNode = stepIdToMat[entry.first];
		matNode->swap(matNode->cloneAndAddParent(entry.second));
		materials.push_back(matNode);
	}

	model->clearIfcModel();

	return true;
}