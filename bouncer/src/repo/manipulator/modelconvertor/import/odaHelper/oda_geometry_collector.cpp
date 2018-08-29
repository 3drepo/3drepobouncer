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


#include "oda_geometry_collector.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace repo::manipulator::modelconvertor::odaHelper;

OdaGeometryCollector::OdaGeometryCollector()
{
	ofile.open("C:\\Users\\Carmen\\Desktop\\testDump.obj");
}


OdaGeometryCollector::~OdaGeometryCollector()
{
}

void OdaGeometryCollector::addMaterialWithColor(const uint32_t &r, const uint32_t &g, const uint32_t &b, const uint32_t &a) {
	uint32_t code = r | (g << 8) | (b << 16) | (a << 24);
	if (codeToMat.find(code) == codeToMat.end())
	{
		repo_material_t mat;
		mat.diffuse = { r / 255.f, g / 255.f, b / 255.f };
		mat.opacity = 1; //a / 255.f; alpha doesn't seem to be used
		codeToMat[code] = repo::core::model::RepoBSONFactory::makeMaterialNode(mat);
	}

	matVector.push_back(code);
}

void OdaGeometryCollector::addMeshEntry(const std::vector<repo::lib::RepoVector3D64> &rawVertices,
	const std::vector<repo_face_t> &faces,
	const std::vector<std::vector<double>> &boundingBox
) {
	if (!minMeshBox.size()) {
		minMeshBox = boundingBox[0];
	}
	else
	{
		minMeshBox[0] = boundingBox[0][0] < minMeshBox[0] ? boundingBox[0][0] : minMeshBox[0];
		minMeshBox[1] = boundingBox[0][1] < minMeshBox[1] ? boundingBox[0][1] : minMeshBox[1];
		minMeshBox[2] = boundingBox[0][2] < minMeshBox[2] ? boundingBox[0][2] : minMeshBox[2];
	}
	meshData.push_back({ rawVertices, faces, 
	{{(float)boundingBox[0][0], (float)boundingBox[0][1], (float)boundingBox[0][2]},
	{ (float)boundingBox[1][0], (float)boundingBox[1][1], (float)boundingBox[1][2] }}, nextMeshName});
	nextMeshName = "";

	for (const auto &v : rawVertices) {
		ofile << "v " << v.x << " " << v.y << " " << v.z << std::endl;
	}

	for (const auto &f : faces) {
		ofile << "f";
		for (const auto &fIdx : f)
			ofile << " " << fIdx + nVectors;
		ofile << std::endl;
	}

	nVectors += rawVertices.size();
}

std::vector<repo::core::model::MeshNode> OdaGeometryCollector::getMeshes() const {
	std::vector<repo::core::model::MeshNode> res;

	res.reserve(meshData.size());
	auto dummyUV = std::vector<std::vector<repo::lib::RepoVector2D>>();
	auto dummyCol = std::vector<repo_color4d_t>();
	auto dummyOutline = std::vector<std::vector<float>>();

	for (const auto &meshEntry : meshData) {
		std::vector<repo::lib::RepoVector3D> vertices32;
		vertices32.reserve(meshEntry.rawVertices.size());

		for (const auto &v : meshEntry.rawVertices) {
			vertices32.push_back({ (float)(v.x - minMeshBox[0]), (float)(v.y - minMeshBox[1]), (float)(v.z - minMeshBox[2]) });
		}

		res.push_back(repo::core::model::RepoBSONFactory::makeMeshNode(
			vertices32, 
			meshEntry.faces, 
			std::vector<repo::lib::RepoVector3D>(), 
			meshEntry.boundingBox,
			dummyUV,
			dummyCol,
			dummyOutline,
			meshEntry.name
		));
	}
	

	return res;
}
