﻿/**
* Allows geometry creation using ifcopenshell
*/

#include "repo_ifc_utils_parser.h"
#include "repo_ifc_utils_constants.h"
#include "ifcUtils/Ifc2x3.h"
#include "ifcUtils/Ifc4.h"

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
	std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials,
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
		convertTreeToNodes(child, meshes, materials, metaSet, transSet, childrenParents);
	}
}

repo::core::model::RepoScene* IFCUtilsParser::generateRepoScene(
	std::string                                                                &errMsg,
	std::unordered_map<std::string, std::vector<repo::core::model::MeshNode*>> &meshes,
	std::unordered_map<std::string, repo::core::model::MaterialNode*>          &materials,
	const std::vector<double>                                                  &offset
)
{
	IfcParse::IfcFile ifcfile(file);

	repoInfo << "IFC Parser initialised.";

	//repoInfo << "Creating Transformations..." << ifcfile.schema()->name();

	TransNode tree;
	bool missingEntities = false;

	if (ifcfile.schema()->name() == "IFC2X3") {
		tree = IfcUtils::Schema_Ifc2x3::TreeParser::createTransformations(file, missingEntities);
	}/*
	else {
		IfcUtils::Schema_Ifc4::TreeParser treeParser(file);
		tree = treeParser.createTransformations();
		missingEntities = treeParser.missingEntities;
	}*/

	repoDebug << "Tree generated. root node is " << tree.name << " with " << tree.children.size() << " children";

	repo::core::model::RepoNodeSet dummy, meshSet, matSet, metaSet, transSet;
	convertTreeToNodes(tree, meshes, materials, metaSet, transSet);

	if (!transSet.size())
	{
		repoError << "Failed to generate a Tree from the IFC file.";
		return nullptr;
	}
	for (auto &m : meshes)
	{
		for (auto &mesh : m.second)
			meshSet.insert(mesh);
	}

	for (auto &m : materials)
	{
		matSet.insert(m.second);
	}

	std::vector<std::string> files = { file };
	repo::core::model::RepoScene *scene = new repo::core::model::RepoScene(files, dummy, meshSet, matSet, metaSet, dummy, transSet);
	scene->setWorldOffset(offset);

	if (missingEntities)
		scene->setMissingNodes();

	return scene;
}