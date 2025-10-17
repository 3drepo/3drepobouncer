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

#include "repo_test_clash_utils.h"

#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/handler/database/repo_query.h>

using namespace testing;
using namespace repo::manipulator::modelutility;

// Unit tests using use the ClashDetectionDatabaseHelper all use the ClashDetection
// database. This is a dump from a fully functional teamspace. Once restored,
// the team_member role can be added to a user allowing the database to be
// inspected or maintained using the frontend.
// If uploading a new file, remember to copy the links into the tests fileShare
// folder as well.

#define TESTDB "ClashDetection"

void ClashDetectionDatabaseHelper::getChildMeshNodes(repo::lib::Container* container, const repo::core::model::RepoBSON& bson, std::set<repo::lib::RepoUUID>& uuids)
{
	if (bson.getStringField(REPO_NODE_LABEL_TYPE) == REPO_NODE_TYPE_MESH)
	{
		uuids.insert(bson.getUUIDField(REPO_NODE_LABEL_ID));
		return;
	}

	auto children = handler->findCursorByCriteria(
		container->teamspace,
		container->container + "." + REPO_COLLECTION_SCENE,
		repo::core::handler::database::query::Eq(REPO_NODE_LABEL_PARENTS, { bson.getUUIDField(REPO_NODE_LABEL_SHARED_ID) })
	);

	for (auto child : *children) {
		getChildMeshNodes(container, child, uuids);
	}
}

// Searches for mesh nodes only
std::set<repo::lib::RepoUUID> ClashDetectionDatabaseHelper::getUniqueIdsByName(
	repo::lib::Container* container,
	std::string name)
{
	std::set<repo::lib::RepoUUID> uuids;

	repo::core::handler::database::query::RepoQueryBuilder query;

	query.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, {
		std::string(REPO_NODE_TYPE_MESH),
		std::string(REPO_NODE_TYPE_TRANSFORMATION)
		}));
	query.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_NAME, name));

	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.includeField(REPO_NODE_LABEL_ID);
	projection.includeField(REPO_NODE_LABEL_SHARED_ID);

	auto cursor = handler->findCursorByCriteria(
		container->teamspace,
		container->container + "." + REPO_COLLECTION_SCENE,
		query,
		projection
	);

	for (auto& bson : *cursor)
	{
		getChildMeshNodes(container, bson, uuids);
	}

	return uuids;
}

CompositeObject ClashDetectionDatabaseHelper::createCompositeObject(
	repo::lib::Container* container,
	std::string name
)
{
	CompositeObject composite;
	composite.id = repo::lib::RepoUUID::createUUID();
	for (auto& uuid : getUniqueIdsByName(container, name))
	{
		composite.meshes.push_back(MeshReference(container, uuid));
	}
	return composite;
}

// Will replace anything in the existing sets
void ClashDetectionDatabaseHelper::setCompositeObjectSetsByName(
	ClashDetectionConfig& config,
	const std::unique_ptr<repo::lib::Container>& container,
	std::initializer_list<std::string> a,
	std::initializer_list<std::string> b)
{
	config.setA.clear();
	for (const auto& name : a) {
		config.setA.push_back(createCompositeObject(container.get(), name));
	}
	config.setB.clear();
	for (const auto& name : b) {
		config.setB.push_back(createCompositeObject(container.get(), name));
	}
}

std::unique_ptr<repo::lib::Container> ClashDetectionDatabaseHelper::getContainerByName(
	std::string name)
{
	auto container = std::make_unique<repo::lib::Container>();
	container->teamspace = TESTDB;

	auto settings = handler->findOneByCriteria(
		TESTDB,
		"settings",
		repo::core::handler::database::query::Eq(REPO_NODE_LABEL_NAME, name)
	);
	container->container = settings.getStringField(REPO_NODE_LABEL_ID);

	auto revisions = handler->getAllFromCollectionTailable(
		TESTDB,
		container->container + "." + REPO_COLLECTION_HISTORY
	);
	container->revision = revisions[0].getUUIDField(REPO_NODE_LABEL_ID);

	return container;
}

void ClashDetectionDatabaseHelper::setCompositeObjectsByMetadataValue(
	repo::manipulator::modelutility::ClashDetectionConfig& config,
	const std::unique_ptr<repo::lib::Container>& container,
	const std::string& valueSetA,
	const std::string& valueSetB)
{
	config.setA.clear();
	createCompositeObjectsByMetadataValue(config.setA, container.get(), valueSetA);
	config.setB.clear();
	createCompositeObjectsByMetadataValue(config.setB, container.get(), valueSetB);
}

void ClashDetectionDatabaseHelper::createCompositeObjectsByMetadataValue(
	std::vector<repo::manipulator::modelutility::CompositeObject>& objects,
	repo::lib::Container* container,
	const std::string& value)
{

	repo::core::handler::database::query::RepoQueryBuilder query;
	query.append(repo::core::handler::database::query::Eq("metadata.value", value));

	repo::core::handler::database::query::RepoProjectionBuilder projection;
	projection.includeField(REPO_NODE_LABEL_PARENTS);

	auto cursor = handler->findCursorByCriteria(
		container->teamspace,
		container->container + "." + REPO_COLLECTION_SCENE,
		query,
		projection
	);

	for (auto& bson : *cursor)
	{
		auto parents = handler->findCursorByCriteria(
			container->teamspace,
			container->container + "." + REPO_COLLECTION_SCENE,
			repo::core::handler::database::query::Eq(REPO_NODE_LABEL_SHARED_ID, bson.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS))
		);

		std::set<repo::lib::RepoUUID> uuids;

		for (auto& parentBson : *parents) {
			getChildMeshNodes(container, parentBson, uuids);
		}

		CompositeObject composite;
		composite.id = repo::lib::RepoUUID::createUUID();
		for (auto& uuid : uuids)
		{
			composite.meshes.push_back(MeshReference(container, uuid));
		}

		objects.push_back(composite);
	}
}