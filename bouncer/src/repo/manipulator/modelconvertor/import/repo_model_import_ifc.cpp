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
#include "repo/manipulator/modelutility/repo_scene_builder.h"
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/error_codes.h"
#include <boost/filesystem.hpp>
#include <ifcparse/IfcFile.h>

#include <ifcgeom/Iterator.h>
#include <ifcgeom/IfcGeomElement.h>
#include <ifcgeom/ConversionSettings.h>
#include "repo/lib/datastructure/repo_structs.h"

#include <ranges>

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

repo::lib::repo_color3d_t repoColor(const ifcopenshell::geometry::taxonomy::colour& c)
{
	return repo::lib::repo_color3d_t(c.r(), c.g(), c.b());
}

bool IFCModelImport::importModel(std::string filePath, uint8_t& err)
{
	throw repo::lib::RepoException("Classic import is no longer supported for IFCs. Please use the streaming import.");
}

void configureDefaultSettings(ifcopenshell::geometry::Settings& settings)
{
	// Look at the ConversionSettings.h file, or the Python documentation, for the
	// set of options. Here we override only those that have a different default
	// value to what we want.

	settings.get<ifcopenshell::geometry::settings::WeldVertices>().value = false; // Welding vertices can make uv projection difficult
	settings.get<ifcopenshell::geometry::settings::UseWorldCoords>().value = true; // Put everything in project coordinates by the time it gets out of the triangulator
	settings.get<ifcopenshell::geometry::settings::GenerateUvs>().value = true;
	settings.get<ifcopenshell::geometry::settings::UseElementHierarchy>().value = true;
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
	std::unordered_map<std::string, repo::lib::repo_material_t> materials;

	repo::lib::RepoBounds getBounds()
	{
		IfcGeom::Iterator boundsIterator(kernel, settings, &file, {}, numThreads);
		boundsIterator.initialize();
		boundsIterator.compute_bounds(false); // False here means the geometry is not triangulated but we get more approximate bounds based on the transforms
		repo::lib::RepoBounds bounds(repoVector(boundsIterator.bounds_min()), repoVector(boundsIterator.bounds_max()));
		return bounds;
	}

	const repo::lib::repo_material_t& resolveMaterial(const ifcopenshell::geometry::taxonomy::style::ptr ptr)
	{
		auto name = ptr->name;
		IfcUtil::sanitate_material_name(name);

		if (materials.find(name) == materials.end())
		{
			repo::lib::repo_material_t material = repo::lib::repo_material_t::DefaultMaterial();

			material.diffuse = repoColor(ptr->get_color());

			material.specular = repoColor(ptr->specular);
			if (ptr->has_specularity()) 
			{
				material.shininess = ptr->specularity;
			}
			else
			{
				material.shininess = 0.5f;
			}
			material.shininessStrength = 0.5f;

			if (ptr->has_transparency()) {
				material.opacity = 1 - ptr->transparency;
			}

			materials[name] = material;
		}
		
		return materials[name];
	}

	void createRootNode()
	{
		auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
		builder->addNode(rootNode);
		rootNodeId = rootNode.getSharedID();
	}

	// If createContainer is true, should create a transformationNode for this element,
	// otherwise just return the parents of the parent.

	struct TreeInfo {
		repo::lib::RepoUUID parentId;
	};

	std::unordered_map<int, repo::lib::RepoUUID> sharedIds;

	/*
	* Builds metadata and possibly transformation nodes for this element. If
	* the caller passes false to createTransform, then it is expected any mesh
	* nodes will attach directly to the parents.
	*/
	repo::lib::RepoUUID getParentId(const IfcGeom::Element* element, bool createTransform)
	{
		// If this element already has a transformation node, return its Id. An
		// existing node will already have all its metdata set up, etc.
		
		if (sharedIds.find(element->id()) != sharedIds.end())
		{
			return sharedIds[element->id()];
		}

		// The parents array is a flat list that represents the inheritence as a stack.
		// It is only populated for the leaf element, so we can't use the member of any
		// of the entries themselves.

		// The following snippet finds the first node we have not yet created a
		// transformation node for, and then builds transformation nodes from that
		// one down.

		repo::lib::RepoUUID parentId = rootNodeId;
		int i = 0;
		for (i = element->parents().size() - 1; i >= 0; i--)
		{
			auto ifcId = element->parents()[i]->id();
			if (sharedIds.find(ifcId) != sharedIds.end())
			{
				parentId = sharedIds[ifcId];
				break;
			}
		}

		for (i++; i < element->parents().size(); i++) 
		{
			auto p = element->parents()[i];
			auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode({}, p->name(), { parentId });
			parentId = transform.getSharedID();
			builder->addNode(transform);
			sharedIds[p->id()] = parentId;
		}

		if (createTransform) {
			auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode({}, element->name(), { parentId });
			parentId = transform.getSharedID();
			sharedIds[element->id()] = parentId;
			builder->addNode(transform);
		}

		return parentId;
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
		auto materials = mesh.materials();

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

		if (!facesByMaterial.size()) {
			return;
		}

		bool createTransformNode = true;// facesByMaterial.size() > 1;

		// Creates the transformation node for this element, and its parent(s), 
		// on-demand. In this version of the importer, we always create a
		// transformation node, regardless of how many child meshes there are -
		// this is because it it is possible to tell up front whether a single
		// mesh may eventually get transformation node siblings.

		auto parentId = getParentId(triangulation, createTransformNode);

		std::string name;
		name = triangulation->name();

		for (auto pair : facesByMaterial)
		{
			// Vertices that are not referenced by a particular set of faces will be
			// removed when removeDuplicates is called as part of the commit process.

			auto mesh = repo::core::model::RepoBSONFactory::makeMeshNode(
				vertices,
				pair.second,
				normals,
				{},
				{ uvs },
				{ },
				{ parentId }
			);
			mesh.updateBoundingBox();
			builder->addNode(mesh);
			builder->addMaterialReference(resolveMaterial(materials[pair.first]), mesh.getSharedID());
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