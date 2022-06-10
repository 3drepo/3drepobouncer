/**
* Allows geometry creation using ifcopenshell
*/

#include "repo_ifc_helper_parser.h"
#include "repo_ifc_helper_common.h"
#include <ifcUtils/repo_ifc_utils_constants.h>
#include <ifcUtils/repo_ifc_utils_Ifc2x3.h>
#include <ifcUtils/repo_ifc_utils_Ifc4.h>

#include "../../../../core/model/bson/repo_bson_factory.h"
#include "../../../../core/model/collection/repo_scene.h"
#include "../../../../lib/repo_utils.h"
#include <boost/filesystem.hpp>
#include <algorithm>

using namespace repo::manipulator::modelconvertor::ifcHelper;

IFCUtilsParser::IFCUtilsParser(const std::string &file) :
	file(file)
{
}

IFCUtilsParser::~IFCUtilsParser()
{
}

void convertTreeToNodes(
	const TransNode &tree,
	std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
	repo::core::model::RepoNodeSet                                             &metaSet,
	repo::core::model::RepoNodeSet                                             &transSet,
	const std::vector<repo::lib::RepoUUID>                                     &parents = std::vector<repo::lib::RepoUUID>()

) {
	std::vector<repo::lib::RepoUUID> childrenParents = parents;
	if (tree.createNode) {
		auto transNode = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode(tree.transformation, tree.name, parents));
		childrenParents = { transNode->getSharedID() };
		auto metaParents = childrenParents;
		transSet.insert(transNode);

		if (meshes.find(tree.guid) != meshes.end()) {
			for (auto &mesh : meshes[tree.guid]) {
				*mesh = mesh->cloneAndAddParent(transNode->getSharedID());
				if (tree.isIfcSpace || tree.meshTakeName && mesh->getName().empty()) {
					*mesh = mesh->cloneAndChangeName(tree.name);
					metaParents.push_back(mesh->getSharedID());
				}
			}
		}

		if (tree.meta.size()) {
			metaSet.insert(new repo::core::model::MetadataNode(repo::core::model::RepoBSONFactory::makeMetaDataNode(tree.meta, tree.name, metaParents)));
		}
	}

	for (const auto child : tree.children) {
		convertTreeToNodes(child, meshes, metaSet, transSet, childrenParents);
	}
}

repo::core::model::RepoScene* IFCUtilsParser::generateRepoScene(
	std::string                                                                &errMsg,
	std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
	std::vector<repo::core::model::MaterialNode*>          &materials,
	const std::vector<double>                                                  &offset
)
{
	repoInfo << "IFC Parser initialised.";

	//repoInfo << "Creating Transformations..." << ifcfile.schema()->name();

	TransNode tree;
	bool missingEntities = false;

	switch (getIFCSchema(file)) {
	case IfcSchemaVersion::IFC2x3:
		tree = repo::ifcUtility::Schema_Ifc2x3::TreeParser::createTransformations(file, missingEntities);
		break;
	case IfcSchemaVersion::IFC4:
		tree = repo::ifcUtility::Schema_Ifc4::TreeParser::createTransformations(file, missingEntities);
		break;
	default:
		errMsg = "Unsupported IFC Version";
		return nullptr;
	}

	repoDebug << "Tree generated. root node is " << tree.name << " with " << tree.children.size() << " children";

	repo::core::model::RepoNodeSet dummy, meshSet, matSet(materials.begin(), materials.end()), metaSet, transSet;
	convertTreeToNodes(tree, meshes, metaSet, transSet);

	if (!transSet.size())
	{
		repoError << "Failed to generate a Tree from the IFC file.";
		return nullptr;
	}
	for (auto &m : meshes)
	{
		for (auto &mesh : m.second) {
			if (!mesh->getParentIDs().size()) {
				repoWarning << "Zombie mesh found with guid: " << m.first;
			}
			meshSet.insert(mesh);
		}
	}

	std::vector<std::string> files = { file };
	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene(files, dummy, meshSet, matSet, metaSet, dummy, transSet);
	scene->setWorldOffset(offset);

	if (missingEntities)
		scene->setMissingNodes();

	return scene;
}