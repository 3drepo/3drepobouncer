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
* Allows Import/Export functionality into/output Repo world using IFCOpenShell
*/

#include "repo_model_import_ifc.h"
//#include "ifcHelper/repo_ifc_helper_geometry.h"
//#include "ifcHelper/repo_ifc_helper_parser.h"
#include "repo/manipulator/modelutility/repo_scene_builder.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/error_codes.h"
#include <boost/filesystem.hpp>
#include <ifcparse/IfcFile.h>

#include <ifcgeom/Iterator.h>
#include <ifcgeom/IfcGeomElement.h>
#include <ifcgeom/ConversionSettings.h>
#include "repo/lib/datastructure/repo_structs.h"


using namespace repo::manipulator::modelconvertor;

IFCModelImport::IFCModelImport(const ModelImportConfig &settings) :
	AbstractModelImport(settings),
	partialFailure(false),
	scene(nullptr)
{
	modelUnits = ModelUnits::METRES;
}

IFCModelImport::~IFCModelImport()
{
}

repo::lib::RepoVector3D64 repoVector(const ifcopenshell::geometry::taxonomy::point3& p)
{
	return repo::lib::RepoVector3D64(p.components_->x(), p.components_->y(), p.components_->z());
}

bool IFCModelImport::importModel(std::string filePath, uint8_t& err)
{
	throw repo::lib::RepoException("Classic import is no longer supported for IFCs. Please use the streaming import.");
}


void configureDefaultSettings(ifcopenshell::geometry::Settings& settings)
{
	settings.get<ifcopenshell::geometry::settings::WeldVertices>().value = false;
	settings.get<ifcopenshell::geometry::settings::UseWorldCoords>().value = true;
	settings.get<ifcopenshell::geometry::settings::ConvertBackUnits>().value = false; // Export in meters
	settings.get<ifcopenshell::geometry::settings::ApplyDefaultMaterials>().value = true;

	/*
	itSettings.set(IfcGeom::WELD_VERTICES, false);
	itSettings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS, true);
	itSettings.set(IfcGeom::IteratorSettings::CONVERT_BACK_UNITS, false);
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
	itSettings.set(IfcGeom::IteratorSettings::INCLUDE_CURVES, false);
	*/
}

#pragma optimize("",off)

class IFCSerialiser
{
public:
	IFCSerialiser(IfcParse::IfcFile& file, const ifcopenshell::geometry::Settings& settings, repo::manipulator::modelutility::RepoSceneBuilder* builder)
		:file(file),
		settings(settings),
		builder(builder)
	{
		kernel = "opencascade";
		numThreads = std::thread::hardware_concurrency();
		createRootNode();
	}

	IfcParse::IfcFile& file;
	const ifcopenshell::geometry::Settings& settings;
	std::string kernel;
	int numThreads;
	repo::manipulator::modelutility::RepoSceneBuilder* builder;
	repo::lib::RepoUUID rootNodeId;

	repo::lib::RepoBounds getBounds()
	{
		IfcGeom::Iterator boundsIterator(kernel, settings, &file, {}, numThreads);
		boundsIterator.initialize();
		boundsIterator.compute_bounds(false); // False here means the geometry is not triangulated but we get more approximate bounds based on the transforms
		repo::lib::RepoBounds bounds(repoVector(boundsIterator.bounds_min()), repoVector(boundsIterator.bounds_max()));
		return bounds;
	}

	void createRootNode()
	{
		auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
		builder->addNode(rootNode);
		rootNodeId = rootNode.getSharedID();
	}

	void import(const IfcGeom::TriangulationElement* triangulation)
	{
		auto& mesh = triangulation->geometry();

		std::vector<repo::lib::RepoVector3D> vertices;
		vertices.reserve(mesh.verts().size() / 3);
		for (auto it = mesh.verts().begin(); it != mesh.verts().end();) {
			const float x = *(it++);
			const float y = *(it++);
			const float z = *(it++);
			vertices.push_back({ x, y, z });
		}

		std::vector<repo::lib::RepoVector3D> normals;
		normals.reserve(mesh.normals().size() / 3);
		for (auto it = mesh.normals().begin(); it != mesh.normals().end();) {
			const float x = *(it++);
			const float y = *(it++);
			const float z = *(it++);
			normals.push_back({ x, y, z });
		}

		std::vector<repo::lib::RepoVector2D> uvs;
		normals.reserve(mesh.uvs().size() / 2);
		for (auto it = mesh.uvs().begin(); it != mesh.uvs().end();) {
			const float u = *it++;
			const float v = *it++;
			uvs.push_back({ u, v });
		}

		auto& facesIt = mesh.faces();
		auto facesMaterialIt = mesh.material_ids().begin();

		std::unordered_map<size_t, std::vector<repo::lib::repo_face_t>> facesByMaterial;		
		for (auto it = facesIt.begin(); it != facesIt.end();)
		{
			const int materialId = *(facesMaterialIt++);
			auto& faces = facesByMaterial[materialId];			
			const size_t v1 = *(it++);
			const size_t v2 = *(it++);
			const size_t v3 = *(it++);
			faces.push_back({ v1, v2, v3 });
		}

		for (auto pair : facesByMaterial)
		{
			// Vertices that are not referenced by a particular set of faces will be
			// removed when removeDuplicates is called as part of the commit process.

			auto mesh = repo::core::model::RepoBSONFactory::makeMeshNode(
				vertices,
				pair.second,
				normals,
				{},
				{ uvs }
			);
			mesh.updateBoundingBox();

			auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode({}, triangulation->name(), { rootNodeId });

			mesh.addParent(transform.getSharedID());

			builder->addNode(transform);
			builder->addNode(mesh);

			builder->addMaterialReference(repo::lib::repo_material_t::DefaultMaterial(), mesh.getSharedID());
		}
	}

	void importGeometry()
	{
		IfcGeom::Iterator contextIterator("opencascade", settings, &file, {}, numThreads);
		int previousProgress = 0;
		if (contextIterator.initialize())
		{
			repoInfo << "Processing geometry...";

			do
			{
				IfcGeom::Element* element = contextIterator.get();
				import(static_cast<const IfcGeom::TriangulationElement*>(element));

				auto progress = contextIterator.progress();
				if (progress != previousProgress) {
					previousProgress = progress;
					repoInfo << progress << "%...";
				}

			} while (contextIterator.next());

			repoInfo << "100%...";
		}
	}
};

repo::core::model::RepoScene* IFCModelImport::generateRepoScene(uint8_t& errCode)
{
	return scene;
}

bool IFCModelImport::importModel(std::string filePath, std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler, uint8_t& err)
{
	std::string fileName = getFileName(filePath);

	repoInfo << "IMPORT [" << fileName << "]";
	repoInfo << "=== IMPORTING MODEL WITH IFC OPEN SHELL ===";

	auto sceneBuilder = std::make_unique<repo::manipulator::modelutility::RepoSceneBuilder>(
		handler,
		settings.getDatabaseName(),
		settings.getProjectName(),
		settings.getRevisionId()
		);
	sceneBuilder->createIndexes();

	IfcParse::IfcFile file(filePath);

	if (!file.good()) {
		throw repo::lib::RepoImportException(REPOERR_MODEL_FILE_READ);
	}

	ifcopenshell::geometry::Settings geometrySettings;
	configureDefaultSettings(geometrySettings);

	IFCSerialiser serialiser(file, geometrySettings, sceneBuilder.get());

	repoInfo << "Computing bounds...";

	auto bounds = serialiser.getBounds();
	sceneBuilder->setWorldOffset(bounds.min());	
	geometrySettings.get<ifcopenshell::geometry::settings::ModelOffset>().value = (-sceneBuilder->getWorldOffset()).toStdVector();

	// Consider setting an HDF5 cache file here to reduce memory
	// Check out IfcConvert.cpp ln 980

	serialiser.importGeometry();

	sceneBuilder->finalise();

	scene = new repo::core::model::RepoScene(
		settings.getDatabaseName(),
		settings.getProjectName()
	);

	scene->setRevision(settings.getRevisionId());
	scene->setOriginalFiles({ filePath });
	scene->loadRootNode(handler.get());
	scene->setWorldOffset(sceneBuilder->getWorldOffset());

	if (sceneBuilder->hasMissingTextures()) {
		scene->setMissingTexture();
	}

	return true;
}