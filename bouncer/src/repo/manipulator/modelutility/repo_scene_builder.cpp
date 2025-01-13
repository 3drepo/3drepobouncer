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

#include "repo_scene_builder.h"

#include "repo/core/model/bson/repo_node_transformation.h"
#include "repo/core/model/bson/repo_node_metadata.h"
#include "repo/core/model/bson/repo_node_mesh.h"
#include "repo/core/model/bson/repo_node_material.h"
#include "repo/core/model/bson/repo_node_texture.h"
#include "repo/core/model/bson/repo_bson.h"

using namespace repo::manipulator::modelutility;
using namespace repo::core::model;

struct RepoSceneBuilder::Deleter
{
	RepoSceneBuilder* builder;
	void operator()(RepoNode* n)
	{
		builder->queueNode(n);
		builder->referenceCounter--;
	}
};

RepoSceneBuilder::RepoSceneBuilder(
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
	std::string database,
	std::string project)
	:handler(handler),
	databaseName(database),
	projectName(project),
	referenceCounter(0)
{
	// A builder must start with a revision node. In the future, we expect this
	// node to be created by .io.
	revisionId = repo::lib::RepoUUID::createUUID();
}

RepoSceneBuilder::~RepoSceneBuilder()
{
	if (referenceCounter != 0)
	{
		throw std::runtime_error("RepoSceneBuilder is being destroyed with outstanding RepoNode references. Make sure all RepoNodes have gone out of scope before RepoSceneBuilder.");
	}

	if (nodesToCommit.size())
	{
		throw std::runtime_error("RepoSceneBuilder is being destroyed with outstanding nodes to commit. Call finalise() before letting RepoSceneBuilder go out of scope.");
	}
}

template<RepoNodeClass T>
std::shared_ptr<T> RepoSceneBuilder::addNode(const T& node)
{
	Deleter deleter;
	deleter.builder = this;
	auto ptr = std::shared_ptr<T>(new T(node), deleter);
	ptr->setRevision(revisionId);
	referenceCounter++;
	return ptr;
}

void RepoSceneBuilder::queueNode(const RepoNode* node)
{
	nodesToCommit.push_back(node);

	if (nodesToCommit.size() > 1000)
	{
		commitNodes();
	}
}

void RepoSceneBuilder::commitNodes()
{
	std::vector< repo::core::model::RepoBSON> bsons;
	for (auto n : nodesToCommit) {
		bsons.push_back(*n);
		delete n;
	}
	nodesToCommit.clear();
	handler->insertManyDocuments(databaseName, projectName + "." + REPO_COLLECTION_SCENE, bsons);
}

void RepoSceneBuilder::finalise()
{
	commitNodes();
}

// It is required to tell the module which specialisations to instantiate
// for addNode.

#define DECLARE_ADDNODE_SPECIALISATION(T) template std::shared_ptr<T> RepoSceneBuilder::addNode<T>(const T& node);

DECLARE_ADDNODE_SPECIALISATION(MeshNode)
DECLARE_ADDNODE_SPECIALISATION(TransformationNode)
DECLARE_ADDNODE_SPECIALISATION(MaterialNode)
DECLARE_ADDNODE_SPECIALISATION(TextureNode)
DECLARE_ADDNODE_SPECIALISATION(MetadataNode)
