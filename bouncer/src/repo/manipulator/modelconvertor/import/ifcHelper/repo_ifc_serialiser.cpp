/**
*  Copyright (C) 2025 3D Repo Ltd
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

#include "repo_ifc_serialiser.h"

#include "repo/lib/datastructure/repo_structs.h"
#include "repo/core/model/bson/repo_bson_factory.h"

#pragma optimize("",off)

using namespace repo::manipulator::modelconvertor::ifcHelper;

repo::lib::RepoVector3D64 repoVector(const ifcopenshell::geometry::taxonomy::point3& p)
{
	return repo::lib::RepoVector3D64(p.components_->x(), p.components_->y(), p.components_->z());
}

repo::lib::repo_color3d_t repoColor(const ifcopenshell::geometry::taxonomy::colour& c)
{
	return repo::lib::repo_color3d_t(c.r(), c.g(), c.b());
}

repo::lib::RepoMatrix repoMatrix(const ifcopenshell::geometry::taxonomy::matrix4& m)
{
	return repo::lib::RepoMatrix(m.ccomponents().data(), false);
}

IFCSerialiser::IFCSerialiser(IfcParse::IfcFile& file, repo::manipulator::modelutility::RepoSceneBuilder* builder)
	:file(file),
	builder(builder)
{
	kernel = "opencascade";
	numThreads = std::thread::hardware_concurrency();
	configureSettings();
	createRootNode();
}

void IFCSerialiser::configureSettings()
{
	// Look at the ConversionSettings.h file, or the Python documentation, for the
	// set of options. Here we override only those that have a different default
	// value to what we want.

	settings.get<ifcopenshell::geometry::settings::WeldVertices>().value = false; // Welding vertices can make uv projection difficult
	settings.get<ifcopenshell::geometry::settings::GenerateUvs>().value = true;
}

void IFCSerialiser::updateBounds()
{
	repoInfo << "Computing bounds...";

	auto bounds = getBounds();
	builder->setWorldOffset(bounds.min());
	settings.get<ifcopenshell::geometry::settings::ModelOffset>().value = (-builder->getWorldOffset()).toStdVector();
}

repo::lib::RepoBounds IFCSerialiser::getBounds()
{
	IfcGeom::Iterator boundsIterator(kernel, settings, &file, {}, numThreads);
	boundsIterator.initialize();
	boundsIterator.compute_bounds(false); // False here means the geometry is not triangulated but we get more approximate bounds based on the transforms
	repo::lib::RepoBounds bounds(repoVector(boundsIterator.bounds_min()), repoVector(boundsIterator.bounds_max()));
	return bounds;
}

const repo::lib::repo_material_t& IFCSerialiser::resolveMaterial(const ifcopenshell::geometry::taxonomy::style::ptr ptr)
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

void IFCSerialiser::createRootNode()
{
	auto rootNode = repo::core::model::RepoBSONFactory::makeTransformationNode({}, "rootNode", {});
	builder->addNode(rootNode);
	rootNodeId = rootNode.getSharedID();
}

/*
* Given an arbitrary Ifc Object, determine from its relationships which is the
* best one for it so it under in the acyclic tree.
*/
const IfcSchema::IfcObjectDefinition* IFCSerialiser::getParent(const IfcSchema::IfcObjectDefinition* object)
{
	auto elem = object->as<IfcSchema::IfcElement>();
	if (elem)
	{
		auto structures = elem->ContainedInStructure();
		if (structures->size()) {
			auto container = *structures->begin();
			return container->RelatingStructure();
		}
	}

	auto compositions = object->file_->getInverse(object->id(), &(IfcSchema::IfcRelDecomposes::Class()), -1);
	for (auto& composition : *compositions) {
		auto whole = composition->as<IfcSchema::IfcRelDecomposes>()->RelatingObject();
		if (!whole || whole == object) {
			continue;
		}
		return whole;
	}

	return nullptr;
}

repo::lib::RepoUUID IFCSerialiser::createTransformationNode(const IfcSchema::IfcObjectDefinition* parent, const IfcParse::entity& type)
{
	auto parentId = createTransformationNode(parent);

	auto groupId = parent->file_->getMaxId() + parent->id() + type.type() + 1;
	if (sharedIds.find(groupId) != sharedIds.end()) {
		return sharedIds[groupId];
	}

	auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode({}, type.name(), { parentId });
	sharedIds[groupId] = transform.getSharedID();;
	builder->addNode(transform);

	return transform.getSharedID();
}

bool IFCSerialiser::shouldGroupByType(const IfcSchema::IfcObjectDefinition* object, const IfcSchema::IfcObjectDefinition* parent)
{
	return parent && !parent->as<IfcSchema::IfcElement>() && object->as<IfcSchema::IfcElement>();
}

repo::lib::RepoUUID IFCSerialiser::createTransformationNode(const IfcSchema::IfcObjectDefinition* object, const repo::lib::RepoMatrix& matrix)
{
	if (!object) {
		return rootNodeId;
	}

	// If this object already has a transformation node, return its Id. An
	// existing node will already have all its metdata set up, etc.

	if (sharedIds.find(object->id()) != sharedIds.end())
	{
		return sharedIds[object->id()];
	}

	auto parent = getParent(object);

	auto parentId = createTransformationNode(parent, {});

	// If the parent is a non-physical container, group elements by type, as
	// a convenience.

	if (shouldGroupByType(object, parent))
	{
		parentId = createTransformationNode(parent, object->declaration());
	}

	auto name = object->Name().get_value_or({});
	if (name.empty())
	{
		name = object->declaration().name();
	}

	auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode(matrix, name, { parentId });
	parentId = transform.getSharedID();
	sharedIds[object->id()] = parentId;
	builder->addNode(transform);

	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;
	metadata["Entity"] = object->declaration().name();
	auto metadataNode = repo::core::model::RepoBSONFactory::makeMetaDataNode(metadata, {}, { parentId });
	builder->addNode(metadataNode);

	return parentId;
}

std::set<repo::lib::RepoUUID> IFCSerialiser::createMeshNodes(
	const IfcGeom::Representation::Triangulation& mesh,
	repo::lib::RepoUUID parentId,
	std::string name,
	repo::lib::RepoMatrix matrix
)
{
	std::set<repo::lib::RepoUUID> uniqueIds;

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
			name,
			{ parentId }
		);
		mesh.updateBoundingBox();
		mesh.applyTransformation(matrix);
		builder->addNode(mesh);
		builder->addMaterialReference(resolveMaterial(materials[pair.first]), mesh.getSharedID());

		uniqueIds.insert(mesh.getUniqueID());
	}

	return uniqueIds;
}

repo::lib::RepoUUID IFCSerialiser::getParentId(const IfcGeom::Element* element, bool createTransform)
{
	auto object = element->product()->as<IfcSchema::IfcObjectDefinition>();
	if (createTransform)
	{
		return createTransformationNode(object, repoMatrix(*element->transformation().data()));
	}
	else
	{
		auto parent = getParent(object);
		if (shouldGroupByType(object, parent))
		{
			return createTransformationNode(parent, object->declaration());
		}
		else
		{
			return createTransformationNode(parent);
		}
	}
}

void IFCSerialiser::import(const IfcGeom::TriangulationElement* element)
{
	// We can't know ahead of time how many references a mesh may have, so all
	// MeshNodes are instanced, by default, except for those we explicitly choose
	// to bake (e.g. Ifc Spaces) (which may get baked over and over).

	// There are only two options: bake the transform into a named mesh, or have an
	// unnamed mesh under a leaf TransformationNode. The importer will not place a
	// baked mesh under a leaf TransformationNode.

	bool instanced = true;

	std::string name;
	if (element->product()->as<IfcSchema::IfcSpace>()) {
		name += " (IFC Space)";
		instanced = false;
	}

	auto& mesh = element->geometry();
	auto matrix = repoMatrix(*element->transformation().data());
	auto parentId = getParentId(element, instanced);

	if (!instanced)
	{
		createMeshNodes(mesh, parentId, name, matrix);
	}
	else
	{
		auto existing = representations.find(mesh.id());
		if (existing != representations.end()) {
			for (auto& uniqueId : existing->second) {
				builder->addParent(uniqueId, parentId);
			}
		}
		else
		{
			representations[mesh.id()] = createMeshNodes(mesh, parentId, {}, {});
		}
	}
}

/*
* The custom filter object is used with the geometry iterator to filter out
* any products we do not want.
*
* This implementation ignores Openings.
*
* IfcSpaces are imported, but appear under their own groups, and so are handled
* further in.
*/
struct filter
{
	bool operator()(IfcUtil::IfcBaseEntity* prod) const {
		if (prod->as<IfcSchema::IfcOpeningElement>()) {
			return false;
		}
		return true;
	}
};

void IFCSerialiser::import()
{
	filter f;
	IfcGeom::Iterator contextIterator("opencascade", settings, &file, { boost::ref(f) }, numThreads);
	int previousProgress = 0;
	if (contextIterator.initialize())
	{
		repoInfo << "Processing geometry with " << numThreads << " threads...";

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

		repoInfo << "100%";
	}
}



