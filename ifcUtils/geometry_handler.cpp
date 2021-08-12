/**
*  Copyright (C) 2021 3D Repo Ltd
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

#define TO_STRING(x) #x
#include <string>
#define INCLUDE_HEADER(x) TO_STRING(x.h)
const std::string headerFile = INCLUDE_HEADER(IfcSchema);
const std::string schemaUsed = TO_STRING(IfcSchema);
#include INCLUDE_HEADER(IfcSchema)
#undef INCLUDE_HEADER

#define SCHEMA_NS CREATE_SCHEMA_NS(Schema_)

#include <ifcgeom/IfcGeom.h>
#include <ifcgeom_schema_agnostic/IfcGeomIterator.h>

IfcGeom::IteratorSettings createSettings()
{
	IfcGeom::IteratorSettings itSettings;

	itSettings.set(IfcGeom::IteratorSettings::WELD_VERTICES, false);
	itSettings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS, true);
	itSettings.set(IfcGeom::IteratorSettings::CONVERT_BACK_UNITS, true);
	itSettings.set(IfcGeom::IteratorSettings::USE_BREP_DATA, false);
	itSettings.set(IfcGeom::IteratorSettings::SEW_SHELLS, false);
	itSettings.set(IfcGeom::IteratorSettings::FASTER_BOOLEANS, false);
	itSettings.set(IfcGeom::IteratorSettings::DISABLE_OPENING_SUBTRACTIONS, false);
	itSettings.set(IfcGeom::IteratorSettings::DISABLE_TRIANGULATION, false);
	itSettings.set(IfcGeom::IteratorSettings::APPLY_DEFAULT_MATERIALS, true);
	itSettings.set(IfcGeom::IteratorSettings::EXCLUDE_SOLIDS_AND_SURFACES, false);
	itSettings.set(IfcGeom::IteratorSettings::NO_NORMALS, false);
	itSettings.set(IfcGeom::IteratorSettings::GENERATE_UVS, true);
	itSettings.set(IfcGeom::IteratorSettings::APPLY_LAYERSETS, false);
	//Enable to get 2D lines. You will need to set CONVERT_BACK_UNITS to false or the model may not align.
	//itSettings.set(IfcGeom::IteratorSettings::INCLUDE_CURVES, true);

	return itSettings;
}

repo_material_t createMaterial(
	const IfcGeom::Material &material) {
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

bool IfcUtils::SCHEMA_NS::GeometryHandler::retrieveGeometry(
	const std::string &file,
	std::vector < std::vector<double>> &allVertices,
	std::vector<std::vector<repo_face_t>> &allFaces,
	std::vector < std::vector<double>> &allNormals,
	std::vector < std::vector<double>> &allUVs,
	std::vector<std::string> &allIds,
	std::vector<std::string> &allNames,
	std::vector<std::string> &allMaterials,
	std::unordered_map<std::string, repo_material_t> &matNameToMaterials,
	std::vector<double>		&offset,
	std::string              &errMsg)
{
	IfcParse::IfcFile ifcfile(file);
	auto itSettings = createSettings();

	IfcGeom::Iterator<double> contextIterator(itSettings, &ifcfile);
	repoInfo << "Generating geometry, IfcSchema: " << schemaUsed << " header file used: " << headerFile;

	try {
		if (!contextIterator.initialize()) {
			errMsg = "Failed to initialise Geometry Iterator";
			return false;
		}
	}
	catch (const std::exception &e)
	{
		errMsg = e.what();
		return false;
	}

	do
	{
		IfcGeom::Element<double> *ob = contextIterator.get();
		if (ifcfile.instance_by_guid(ob->guid())->data().type()->name_lc() == "ifcopeningelement") continue;

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
						std::string matName = material.original_name();
						post_materials[matInd] = matName;
						if (matNameToMaterials.find(matName) == matNameToMaterials.end())
						{
							//new material, add it to the vector
							repo_material_t matProp = createMaterial(material);
							matNameToMaterials[matName] = matProp;
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

	return true;
}