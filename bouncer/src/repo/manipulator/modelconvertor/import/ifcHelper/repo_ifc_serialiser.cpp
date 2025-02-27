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
#include "repo_ifc_constants.h"

#include "repo/lib/datastructure/repo_structs.h"
#include "repo/lib/datastructure/repo_variant.h"
#include "repo/core/model/bson/repo_bson_factory.h"

#include <ifcparse/IfcEntityInstanceData.h>

#pragma optimize("",off)

using namespace repo::manipulator::modelconvertor::ifcHelper;

/*
* The starting index of the subtype attributes - this is the same for Ifc2x3 and
* 4x, however if it changes in future versions we may have to get it from the
* schema instead.
*/
#define NUM_ROOT_ATTRIBUTES 4

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

template<typename T>
std::string stringifyVector(std::vector<T> arr)
{
	std::stringstream ss;
	ss << "(";
	for (size_t i = 0; i < arr.size(); i++) {
		ss << arr[i];
		if (i < arr.size() - 1) {
			ss << ",";
		}
	}
	ss << ")";
	return ss.str();
}

template<typename T>
repo::lib::RepoVariant stringify(std::vector<T> arr)
{
	repo::lib::RepoVariant v;
	if (arr.size()) {
		v = stringifyVector(arr);
	}
	return v;
}

template<typename T>
repo::lib::RepoVariant stringify(std::vector<std::vector<T>> arr)
{
	repo::lib::RepoVariant v;
	if (arr.size()) {
		std::stringstream ss;
		ss << "(";
		for (size_t i = 0; i < arr.size(); i++) {
			ss << stringifyVector(arr[i]);
			if (i < arr.size() - 1) {
				ss << ",";
			}
		}
		ss << ")";
		v = ss.str();
	}
	return v;
}

std::optional<repo::lib::RepoVariant> repoVariant(const AttributeValue& a)
{
	repo::lib::RepoVariant v;
	switch (a.type())
	{
	case IfcUtil::ArgumentType::Argument_NULL:
	case IfcUtil::ArgumentType::Argument_DERIVED: // not clear what this one is...
		return std::nullopt;
	case IfcUtil::ArgumentType::Argument_INT:
		v = a.operator int();
		return v;
	case IfcUtil::ArgumentType::Argument_BOOL:
		v = a.operator bool();
		return v;
	case IfcUtil::ArgumentType::Argument_LOGICAL:
	{
		auto value = a.operator boost::logic::tribool();
		if (value == boost::logic::tribool::true_value) {
			v = true;
		}
		else if (value == boost::logic::tribool::false_value) {
			v = false;
		}
		else {
			v = "UNKNOWN";
		}
		return v;
	}
	case IfcUtil::ArgumentType::Argument_DOUBLE:
		v = a.operator double();
		return v;
	case IfcUtil::ArgumentType::Argument_STRING:
		v = a.operator std::string();
		return v;

	// Aggregations will often hold things such as GIS coordinates; usually these
	// complex types should be formatted explicitly by processAttributeValue, but
	// make a best effort to return something as a fallback if not.

	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_INT:
		v = stringify(a.operator std::vector<int, std::allocator<int>>());
		return v;
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_DOUBLE:
		v = stringify(a.operator std::vector<double, std::allocator<double>>());
		return v;
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_STRING:
		v = stringify(a.operator std::vector<std::string, std::allocator<std::string>>());
		return v;
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_INT:
		v = stringify(a.operator std::vector<std::vector<int, std::allocator<int>>, std::allocator<std::vector<int, std::allocator<int>>>>());
		return v;
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_DOUBLE:
		v = stringify(a.operator std::vector<std::vector<double, std::allocator<double>>, std::allocator<std::vector<double, std::allocator<double>>>>());
		return v;

	// IFCOS will conveniently resolve the enumeration label through the string
	// conversion without us having to do anything else...

	case IfcUtil::ArgumentType::Argument_ENUMERATION:
		v = a.operator std::string();
		return v;

	case IfcUtil::ArgumentType::Argument_BINARY:
	case IfcUtil::ArgumentType::Argument_ENTITY_INSTANCE:
	case IfcUtil::ArgumentType::Argument_EMPTY_AGGREGATE:
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_BINARY:
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_ENTITY_INSTANCE:
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_EMPTY_AGGREGATE:
	case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE:
	case IfcUtil::ArgumentType::Argument_UNKNOWN:
		return std::nullopt;
	}
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
* best one for it to sit under in the acyclic tree.
*/
const IfcSchema::IfcObjectDefinition* IFCSerialiser::getParent(const IfcSchema::IfcObjectDefinition* object)
{
	auto connects = object->file_->getInverse(object->id(), &(IfcSchema::IfcRelContainedInSpatialStructure::Class()), -1);
	for (auto& rel : *connects) {
		auto contained = rel->as<IfcSchema::IfcRelContainedInSpatialStructure>();
		auto structure = contained->RelatingStructure();
		if (!structure || structure == object) {
			continue;
		}
		return structure;
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

repo::lib::RepoUUID IFCSerialiser::createTransformationNode(const IfcSchema::IfcObjectDefinition* object)
{
	if (!object)
	{
		return rootNodeId;
	}

	// If this object already has a transformation node, return its Id. An
	// existing node will already have all its metdata set up, etc.

	if (sharedIds.find(object->id()) != sharedIds.end())
	{
		return sharedIds[object->id()];
	}

	auto parent = getParent(object);

	auto parentId = createTransformationNode(parent);

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

	auto transform = repo::core::model::RepoBSONFactory::makeTransformationNode({}, name, { parentId });
	parentId = transform.getSharedID();
	sharedIds[object->id()] = parentId;
	builder->addNode(transform);

	auto metaNode = createMetadataNode(object);
	metaNode->addParent(parentId);
	builder->addNode(std::move(metaNode));

	return parentId;
}

repo::lib::RepoUUID IFCSerialiser::getParentId(const IfcGeom::Element* element, bool createTransform)
{
	auto object = element->product()->as<IfcSchema::IfcObjectDefinition>();
	if (createTransform)
	{
		return createTransformationNode(object);
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

void IFCSerialiser::import(const IfcGeom::TriangulationElement* triangulation)
{
	auto& mesh = triangulation->geometry();

	auto matrix = repoMatrix(*triangulation->transformation().data());

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

	bool createTransformNode = facesByMaterial.size() > 1;

	std::string name;
	if (!createTransformNode) {
		name = triangulation->name();
	}

	if (triangulation->product()->as<IfcSchema::IfcSpace>()) {
		name += " (IFC Space)";
	}

	auto parentId = getParentId(triangulation, createTransformNode);

	std::unique_ptr<repo::core::model::MetadataNode> metaNode;
	if (!createTransformNode) {
		metaNode = createMetadataNode(triangulation->product()->as<IfcSchema::IfcObjectDefinition>());
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
		mesh.applyTransformation(matrix);
		mesh.updateBoundingBox();
		builder->addNode(mesh);
		builder->addMaterialReference(resolveMaterial(materials[pair.first]), mesh.getSharedID());

		if (metaNode) {
			metaNode->addParent(mesh.getSharedID());
		}
	}

	if (metaNode) {
		builder->addNode(std::move(metaNode));
	}
}

std::string constructMetadataLabel(
	const std::string& label,
	const std::string& prefix,
	const std::string& units = {}
) {
	std::stringstream ss;

	if (!prefix.empty())
		ss << prefix << "::";

	ss << label;

	if (!units.empty()) {
		ss << " (" << units << ")";
	}
	return ss.str();
}

void collectMetadata(const IfcSchema::IfcObjectDefinition* object, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata, std::string group)
{
	if (auto propVal = dynamic_cast<const IfcSchema::IfcPropertySingleValue*>(object)) {
		/*
		std::string value = "n/a";
		std::string units = "";
		if (propVal->NominalValue())
		{
			value = getValueAsString(propVal->NominalValue(), units, projectUnits);
		}

		if (propVal->Unit())
		{
			units = processUnits(propVal->Unit()).second;
		}

		metaValues[constructMetadataLabel(propVal->Name(), metaPrefix, units)] = value;
		*/
	}

	if (auto defByProp = dynamic_cast<const IfcSchema::IfcRelDefinesByProperties*>(object)) {
		/*
		extraChildren.push_back(defByProp->RelatingPropertyDefinition());
		action.cacheMetadata = true;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto propSet = dynamic_cast<const IfcSchema::IfcPropertySet*>(object)) {
		/*
		auto propDefs = propSet->HasProperties();
		extraChildren.insert(extraChildren.end(), propDefs->begin(), propDefs->end());
		childrenMetaPrefix = constructMetadataLabel(propSet->Name(), metaPrefix);

		action.createElement = false;
		action.cacheMetadata = true;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto propSet = dynamic_cast<const IfcSchema::IfcPropertyBoundedValue*>(object)) {
		/*
		auto unitsOverride = propSet->hasUnit() ? processUnits(propSet->Unit()).second : "";
		std::string upperBound, lowerBound;
		if (propSet->hasUpperBoundValue()) {
			std::string units;
			upperBound = getValueAsString(propSet->UpperBoundValue(), units, projectUnits);
			if (unitsOverride.empty()) unitsOverride = units;
		}
		if (propSet->hasLowerBoundValue()) {
			std::string units;
			lowerBound = getValueAsString(propSet->UpperBoundValue(), units, projectUnits);
			if (unitsOverride.empty()) unitsOverride = units;
		}

		metaValues[constructMetadataLabel(propSet->Name(), metaPrefix, unitsOverride)] = "[" + lowerBound + ", " + upperBound + "]";

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto eleQuan = dynamic_cast<const IfcSchema::IfcElementQuantity*>(object)) {
		/*
		auto quantities = eleQuan->Quantities();
		extraChildren.insert(extraChildren.end(), quantities->begin(), quantities->end());
		childrenMetaPrefix = constructMetadataLabel(eleQuan->Name(), metaPrefix);

		action.createElement = false;
		action.takeRefsAsChildren = false;
		action.cacheMetadata = true;
		*/
	}

	if (auto quantities = dynamic_cast<const IfcSchema::IfcPhysicalComplexQuantity*>(object)) {
		/*
		childrenMetaPrefix = constructMetadataLabel(quantities->Name(), metaPrefix);
		auto quantitySet = quantities->HasQuantities();
		extraChildren.insert(extraChildren.end(), quantitySet->begin(), quantitySet->end());

		action.createElement = false;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityLength*>(object)) {
		/*
		const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second :
			getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_LENGTHUNIT, projectUnits);

		metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->LengthValue();

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityArea*>(object)) {
		/*
		const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_AREAUNIT, projectUnits);

		metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->AreaValue();

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityCount*>(object)) {
		/*
		const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : "";
		metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->CountValue();

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityTime*>(object)) {
		/*
		const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_TIMEUNIT, projectUnits);
		metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->TimeValue();

		action.createElement = false;
		action.traverseChildren = false;
		*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityVolume*>(object)) {
		/*
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_VOLUMEUNIT, projectUnits);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->VolumeValue();

			action.createElement = false;
			action.traverseChildren = false;
			*/
	}

	if (auto quantity = dynamic_cast<const IfcSchema::IfcQuantityWeight*>(object)) {
		/*
			const std::string units = quantity->hasUnit() ? processUnits(quantity->Unit()).second : getUnits(IfcSchema::IfcUnitEnum::IfcUnitEnum::IfcUnit_MASSUNIT, projectUnits);

			metaValues[constructMetadataLabel(quantity->Name(), metaPrefix, units)] = quantity->WeightValue();

			action.createElement = false;
			action.traverseChildren = false;
			*/
	}

	if (auto relCS = dynamic_cast<const IfcSchema::IfcRelAssociatesClassification*>(object)) {
		/*
		action.traverseChildren = false;
		action.takeRefsAsChildren = false;
		generateClassificationInformation(relCS, metaValues);
		*/
	}

	if (auto relCS = dynamic_cast<const IfcSchema::IfcRelContainedInSpatialStructure*>(object)) {
		/*
		try {
			auto relatedObjects = relCS->RelatedElements();
			if (relatedObjects)
			{
				extraChildren.insert(extraChildren.end(), relatedObjects->begin(), relatedObjects->end());
			}
		}
		catch (const IfcParse::IfcException& e)
		{
			repoError << "Failed to retrieve related elements from " << relCS->data().id() << ": " << e.what();
			missingEntities = true;
		}
		action.createElement = false;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto relAgg = dynamic_cast<const IfcSchema::IfcRelAggregates*>(object)) {
		/*
		auto relatedObjects = relAgg->RelatedObjects();
		if (relatedObjects)
		{
			extraChildren.insert(extraChildren.end(), relatedObjects->begin(), relatedObjects->end());
		}
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto eleType = dynamic_cast<const IfcSchema::IfcTypeObject*>(object)) {
		/*
		if (eleType->hasHasPropertySets()) {
			auto propSets = eleType->HasPropertySets();
			extraChildren.insert(extraChildren.end(), propSets->begin(), propSets->end());
		}

		action.createElement = false;
		action.traverseChildren = true;
		action.cacheMetadata = true;
		action.takeRefsAsChildren = false;
		*/
	}

	if (auto eleType = dynamic_cast<const IfcSchema::IfcPropertyDefinition*>(object)) {
		/*
		action.createElement = false;
		action.traverseChildren = false;
		action.takeRefsAsChildren = false;
		*/
	}
}

/*
For some types, build explicit metadata entries
*/
void collectAdditionalAttributes(const IfcSchema::IfcObjectDefinition* object, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	if (auto project = dynamic_cast<const IfcSchema::IfcProject*>(object)) {
		metadata[constructMetadataLabel(PROJECT_LABEL, LOCATION_LABEL)] = project->Name().get_value_or("(" + object->declaration().name() + ")");
	}

	if (auto building = dynamic_cast<const IfcSchema::IfcBuilding*>(object)) {
		metadata[constructMetadataLabel(BUILDING_LABEL, LOCATION_LABEL)] = building->Name().get_value_or("(" + object->declaration().name() + ")");
	}

	if (auto storey = dynamic_cast<const IfcSchema::IfcBuildingStorey*>(object)) {
		metadata[constructMetadataLabel(STOREY_LABEL, LOCATION_LABEL)] = storey->Name().get_value_or("(" + object->declaration().name() + ")");
	}
}

struct Formatted {
	std::optional<repo::lib::RepoVariant> v;
	std::string units;
};

Formatted processAttributeValue(const IfcParse::attribute* attribute, const AttributeValue& value, std::unordered_map<std::string, repo::lib::RepoVariant>& metadata)
{
	// Ifc types provide information on how to interpret the underlying values; we
	// can use these to determine units, and explicit formatting where necessary.

	auto type = attribute->type_of_attribute();

	if (type->is(IfcSchema::IfcCompoundPlaneAngleMeasure::Class())) {
		auto c = value.operator std::vector<int, std::allocator<int>>();
		std::stringstream ss;
		ss << c[0] << "° ";
		ss << abs(c[1]) << "' ";
		ss << abs(c[2]) << "\" ";
		ss << abs(c[3]);
		repo::lib::RepoVariant v;
		v = ss.str();
		return { v, {} };
	}
	else {
		// The fallback is simply to report the raw value
		return { repoVariant(value), {} };
	}
}

std::unique_ptr<repo::core::model::MetadataNode> IFCSerialiser::createMetadataNode(const IfcSchema::IfcObjectDefinition* object)
{
	std::unordered_map<std::string, repo::lib::RepoVariant> metadata;

	auto& type = object->declaration();
	auto& instance = object->data();

	metadata[REPO_LABEL_IFC_TYPE] = type.name();
	metadata[REPO_LABEL_IFC_NAME] = object->Name().get_value_or("(" + type.name() + ")");
	metadata[REPO_LABEL_IFC_GUID] = object->GlobalId();
	if (object->Description()) {
		metadata[REPO_LABEL_IFC_DESCRIPTION] = object->Description().value();
	}

	// All types inherit from IfcRoot, which defines four attributes, of which we
	// take three explicitly above. The remaining attributes depend on the subtype.

	for (size_t i = NUM_ROOT_ATTRIBUTES; i < type.attribute_count(); i++)
	{
		auto attribute = type.attribute_by_index(i);
		auto value = object->data().get_attribute_value(i);
		if (!value.isNull()) {
			auto f = processAttributeValue(attribute, value, metadata);
			if (f.v) {
				metadata[attribute->name()] = *f.v;
			}
		}
	}

	collectAdditionalAttributes(object, metadata);

	auto metadataNode = std::make_unique<repo::core::model::MetadataNode>(repo::core::model::RepoBSONFactory::makeMetaDataNode(metadata, {}, {}));
	return metadataNode;
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

		// try get project units here...
		//	if (auto project = dynamic_cast<const IfcSchema::IfcProject*>(object)) {
		//	setProjectUnits(project->UnitsInContext(), projectUnits);
		//	locationData[constructMetadataLabel(PROJECT_LABEL, LOCATION_LABEL)] = project->hasName() ? project->Name() : "(" + element->data().type()->name() + ")";

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



