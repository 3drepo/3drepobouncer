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

#include "repo_ifc_utils_geometry.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include <boost/filesystem.hpp>
#include "repo_model_import_config_default_values.h"
using namespace repo::manipulator::modelconvertor;

IFCUtilsGeometry::IFCUtilsGeometry(const std::string &file, const ModelImportConfig &settings) :
	file(file),
	settings(settings)
{
}

IFCUtilsGeometry::~IFCUtilsGeometry()
{
}

repo_material_t IFCUtilsGeometry::createMaterial(
	const IfcGeom::Material &material)
{
	repo_material_t matProp;
	if (material.hasDiffuse())
	{
		auto diffuse = material.diffuse();
		matProp.diffuse = { (float)diffuse[0], (float)diffuse[1], (float)diffuse[2] };
	}

	if (material.hasSpecular())
	{
		auto specular = material.specular();
		matProp.specular = { (float)specular[0], (float)specular[1], (float)specular[2] };
	}
	else {
		matProp.specular = { 0, 0, 0, 0 };
	}

	if (material.hasSpecularity())
	{
		matProp.shininess = material.specularity() / 256;
	}
	else
	{
		matProp.shininess = 0.5;
	}

	matProp.shininessStrength = 0.5;

	if (material.hasTransparency())
	{
		matProp.opacity = 1. - material.transparency();
	}
	else
	{
		matProp.opacity = 1.;
	}
	return matProp;
}

IfcGeom::IteratorSettings IFCUtilsGeometry::createSettings()
{
	IfcGeom::IteratorSettings itSettings;

	itSettings.set(IfcGeom::IteratorSettings::WELD_VERTICES, repoDefaultIOSWieldVertices);
	itSettings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS, repoDefaultIOSUseWorldCoords);
	itSettings.set(IfcGeom::IteratorSettings::CONVERT_BACK_UNITS, false);
	itSettings.set(IfcGeom::IteratorSettings::USE_BREP_DATA, repoDefaultIOSUseBrepData);
	itSettings.set(IfcGeom::IteratorSettings::SEW_SHELLS, repoDefaultIOSSewShells);
	itSettings.set(IfcGeom::IteratorSettings::FASTER_BOOLEANS, repoDefaultIOSFasterBooleans);
	itSettings.set(IfcGeom::IteratorSettings::DISABLE_OPENING_SUBTRACTIONS, repoDefaultIOSDisableOpeningSubtractions);
	itSettings.set(IfcGeom::IteratorSettings::DISABLE_TRIANGULATION, repoDefaultIOSDisableTriangulate);
	itSettings.set(IfcGeom::IteratorSettings::APPLY_DEFAULT_MATERIALS, repoDefaultIOSApplyDefaultMaterials);
	itSettings.set(IfcGeom::IteratorSettings::EXCLUDE_SOLIDS_AND_SURFACES, repoDefaultIOSExcludesSolidsAndSurfaces);
	itSettings.set(IfcGeom::IteratorSettings::NO_NORMALS, repoDefaultIOSNoNormals);
	itSettings.set(IfcGeom::IteratorSettings::USE_ELEMENT_NAMES, repoDefaultIOSUseElementNames);
	itSettings.set(IfcGeom::IteratorSettings::USE_ELEMENT_GUIDS, repoDefaultIOSUseElementGuids);
	itSettings.set(IfcGeom::IteratorSettings::USE_MATERIAL_NAMES, repoDefaultIOSUseMatNames);
	itSettings.set(IfcGeom::IteratorSettings::CENTER_MODEL, repoDefaultIOSCentreModel);
	itSettings.set(IfcGeom::IteratorSettings::GENERATE_UVS, repoDefaultIOSGenerateUVs);
	itSettings.set(IfcGeom::IteratorSettings::APPLY_LAYERSETS, repoDefaultIOSApplyLayerSets);
	itSettings.set(IfcGeom::IteratorSettings::INCLUDE_CURVES, true);

	return itSettings;
}

bool IFCUtilsGeometry::generateGeometry(
	std::string &errMsg,
	bool        &partialFailure)
{
	partialFailure = false;
	repoInfo << "Initialising Geometry....." << std::endl;

	auto itSettings = createSettings();

	IfcGeom::Iterator<double> contextIterator(itSettings, file);

	auto filter = repoDefaultIfcOpenShellFilterList;
	if (repoDefaultIOSUseFilter && filter.size())
	{
		std::set<std::string> filterSet(filter.begin(), filter.end());

		if (repoDefaultIsExclusion)
		{
			contextIterator.excludeEntities(filterSet);
		}
		else
		{
			contextIterator.includeEntities(filterSet);
		}
	}

	repoTrace << "Initialising Geom iterator";
	int res = IFCOPENSHELL_GEO_INIT_FAILED;
	try {
		res = contextIterator.initialize();
	}
	catch (const std::exception &e)
	{
		repoError << "Failed to initialise Geom iterator: " << e.what();
	}

	if (IFCOPENSHELL_GEO_INIT_SUCCESS)
	{
		repoTrace << "Geom Iterator initialized";
	}
	else if (IFCOPENSHELL_GEO_INIT_PART_SUCCESS)
	{
		repoWarning << "Geom Iterator initialized with part failure";
		partialFailure = true;
	}
	else
	{
		errMsg = "Failed to initialised Geom Iterator";
		return false;
	}

	std::vector<std::vector<repo_face_t>> allFaces;
	std::vector<std::vector<double>> allVertices;
	std::vector<std::vector<double>> allNormals;
	std::vector<std::vector<double>> allUVs;
	std::vector<std::string> allIds, allNames, allMaterials;

	retrieveGeometryFromIterator(contextIterator, itSettings.get(IfcGeom::IteratorSettings::USE_MATERIAL_NAMES),
		allVertices, allFaces, allNormals, allUVs, allIds, allNames, allMaterials);

	//now we have found all meshes, take the minimum bounding box of the scene as offset
	//and create repo meshes
	repoTrace << "Finished iterating. number of meshes found: " << allVertices.size();
	repoTrace << "Finished iterating. number of materials found: " << materials.size();

	std::map<std::string, std::vector<repo::lib::RepoUUID>> materialParent;
	std::string defaultMaterialName = "_3DREPO_DEFAULT_MAT";
	std::vector<repo::lib::RepoUUID> parentsOfDefault;
	for (int i = 0; i < allVertices.size(); ++i)
	{
		std::vector<repo::lib::RepoVector3D> vertices, normals;
		std::vector<repo::lib::RepoVector2D> uvs;
		std::vector<repo_face_t> faces;
		std::vector<std::vector<float>> boundingBox;
		for (int j = 0; j < allVertices[i].size(); j += 3)
		{
			vertices.push_back({ (float)(allVertices[i][j] - offset[0]), (float)(allVertices[i][j + 1] - offset[1]), (float)(allVertices[i][j + 2] - offset[2]) });
			if (allNormals[i].size())
				normals.push_back({ (float)allNormals[i][j], (float)allNormals[i][j + 1], (float)allNormals[i][j + 2] });

			auto vertex = vertices.back();
			if (j == 0)
			{
				boundingBox.push_back({ vertex.x, vertex.y, vertex.z });
				boundingBox.push_back({ vertex.x, vertex.y, vertex.z });
			}
			else
			{
				boundingBox[0][0] = boundingBox[0][0] > vertex.x ? vertex.x : boundingBox[0][0];
				boundingBox[0][1] = boundingBox[0][1] > vertex.y ? vertex.y : boundingBox[0][1];
				boundingBox[0][2] = boundingBox[0][2] > vertex.z ? vertex.z : boundingBox[0][2];

				boundingBox[1][0] = boundingBox[1][0] < vertex.x ? vertex.x : boundingBox[1][0];
				boundingBox[1][1] = boundingBox[1][1] < vertex.y ? vertex.y : boundingBox[1][1];
				boundingBox[1][2] = boundingBox[1][2] < vertex.z ? vertex.z : boundingBox[1][2];
			}
		}

		for (int j = 0; j < allUVs[i].size(); j += 2)
		{
			uvs.push_back({ (float)allUVs[i][j], (float)allUVs[i][j + 1] });
		}

		std::vector < std::vector<repo::lib::RepoVector2D>> uvChannels;
		if (uvs.size())
			uvChannels.push_back(uvs);

		auto mesh = repo::core::model::RepoBSONFactory::makeMeshNode(vertices, allFaces[i], normals, boundingBox, uvChannels,
			std::vector<repo_color4d_t>(), std::vector<std::vector<float>>());

		if (meshes.find(allIds[i]) == meshes.end())
		{
			meshes[allIds[i]] = std::vector<repo::core::model::MeshNode*>();
		}
		meshes[allIds[i]].push_back(new repo::core::model::MeshNode(mesh));

		if (allMaterials[i] != "")
		{
			if (materialParent.find(allMaterials[i]) == materialParent.end())
			{
				materialParent[allMaterials[i]] = std::vector<repo::lib::RepoUUID>();
			}

			materialParent[allMaterials[i]].push_back(mesh.getSharedID());
		}
		else
		{
			//This mesh has no material, assigning a default
			if (materials.find(defaultMaterialName) == materials.end())
			{
				repo_material_t matProp;
				matProp.diffuse = { 0.5, 0.5, 0.5, 1 };
				materials[defaultMaterialName] = (new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(matProp, defaultMaterialName)));
			}
			if (materialParent.find(defaultMaterialName) == materialParent.end())
			{
				materialParent[defaultMaterialName] = std::vector<repo::lib::RepoUUID>();
			}
			materialParent[defaultMaterialName].push_back(mesh.getSharedID());
		}
	}

	repoTrace << "Meshes constructed. Wiring materials to parents...";

	for (const auto &pair : materialParent)
	{
		auto matIt = materials.find(pair.first);
		if (matIt != materials.end())
		{
			*(matIt->second) = matIt->second->cloneAndAddParent(pair.second);
		}
	}

	return true;
}

void IFCUtilsGeometry::retrieveGeometryFromIterator(
	IfcGeom::Iterator<double> &contextIterator,
	const bool useMaterialNames,
	std::vector < std::vector<double>> &allVertices,
	std::vector<std::vector<repo_face_t>> &allFaces,
	std::vector < std::vector<double>> &allNormals,
	std::vector < std::vector<double>> &allUVs,
	std::vector<std::string> &allIds,
	std::vector<std::string> &allNames,
	std::vector<std::string> &allMaterials)
{
	repoTrace << "Iterating through Geom Iterator.";
	do
	{
		IfcGeom::Element<double> *ob = contextIterator.get();
		auto ob_geo = static_cast<const IfcGeom::TriangulationElement<double>*>(ob);
		if (ob_geo)
		{
			auto primitive = 3;
			auto faces = ob_geo->geometry().faces();
			if (!faces.size()) {
				if ((faces = ob_geo->geometry().edges()).size()) {
					primitive = 2;
				}
				else {
					continue;
				}
			}

			auto vertices = ob_geo->geometry().verts();
			auto normals = ob_geo->geometry().normals();
			auto uvs = ob_geo->geometry().uvs();
			std::unordered_map<int, std::unordered_map<int, int>>  indexMapping;
			std::unordered_map<int, int> vertexCount;
			std::unordered_map<int, std::vector<double>> post_vertices, post_normals, post_uvs;
			std::unordered_map<int, std::string> post_materials;
			std::unordered_map<int, std::vector<repo_face_t>> post_faces;

			auto matIndIt = ob_geo->geometry().material_ids().begin();

			for (int i = 0; i < vertices.size(); i += 3)
			{
				for (int j = 0; j < 3; ++j)
				{
					int index = j + i;
					if (offset.size() < j + 1)
					{
						offset.push_back(vertices[index]);
					}
					else
					{
						offset[j] = offset[j] > vertices[index] ? vertices[index] : offset[j];
					}
				}
			}

			for (int iface = 0; iface < faces.size(); iface += primitive)
			{
				auto matInd = primitive == 3 ? *matIndIt : ob_geo->geometry().materials().size();
				if (indexMapping.find(matInd) == indexMapping.end())
				{
					//new material
					indexMapping[matInd] = std::unordered_map<int, int>();
					vertexCount[matInd] = 0;

					std::unordered_map<int, std::vector<double>> post_vertices, post_normals, post_uvs;
					std::unordered_map<int, std::vector<repo_face_t>> post_faces;

					post_vertices[matInd] = std::vector<double>();
					post_normals[matInd] = std::vector<double>();
					post_uvs[matInd] = std::vector<double>();
					post_faces[matInd] = std::vector<repo_face_t>();

					if (matInd < ob_geo->geometry().materials().size()) {
						auto material = ob_geo->geometry().materials()[matInd];
						std::string matName = useMaterialNames ? material.original_name() : material.name();
						post_materials[matInd] = matName;
						if (materials.find(matName) == materials.end())
						{
							//new material, add it to the vector
							repo_material_t matProp = createMaterial(material);
							materials[matName] = new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(matProp, matName));
						}
					}
					else {
						post_materials[matInd] = "";
					}
				}

				repo_face_t face;
				for (int j = 0; j < primitive; ++j)
				{
					auto vIndex = faces[iface + j];
					if (indexMapping[matInd].find(vIndex) == indexMapping[matInd].end())
					{
						//new index. create a mapping
						indexMapping[matInd][vIndex] = vertexCount[matInd]++;
						for (int ivert = 0; ivert < 3; ++ivert)
						{
							auto bufferInd = ivert + vIndex * 3;
							post_vertices[matInd].push_back(vertices[bufferInd]);

							if (normals.size())
								post_normals[matInd].push_back(normals[bufferInd]);

							if (uvs.size() && ivert < 2)
							{
								auto uvbufferInd = ivert + vIndex * 2;
								post_uvs[matInd].push_back(uvs[uvbufferInd]);
							}
						}
					}
					face.push_back(indexMapping[matInd][vIndex]);
				}

				post_faces[matInd].push_back(face);

				if (primitive == 3) ++matIndIt;
			}

			auto guid = ob_geo->guid();
			auto name = ob_geo->name();
			for (const auto& pair : post_faces)
			{
				auto index = pair.first;
				allVertices.push_back(post_vertices[index]);
				allNormals.push_back(post_normals[index]);
				allFaces.push_back(pair.second);
				allUVs.push_back(post_uvs[index]);

				allIds.push_back(guid);
				allNames.push_back(name);
				allMaterials.push_back(post_materials[index]);
			}
		}
		if (allIds.size() % 100 == 0)
			repoInfo << allIds.size() << " meshes created";
	} while (contextIterator.next());
}