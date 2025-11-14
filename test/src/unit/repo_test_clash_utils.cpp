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

#include "repo_test_utils.h"
#include "repo_test_clash_utils.h"
#include "repo_test_database_info.h"

#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_bson_factory.h>
#include <repo/core/handler/database/repo_query.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_manager.h>
#include <repo/manipulator/modelutility/clashdetection/geometry_tests.h>

using namespace testing;
using namespace repo::manipulator::modelutility;
using namespace repo::lib;

// Unit tests using use the ClashDetectionDatabaseHelper all use the ClashDetection
// database. This is a dump from a fully functional teamspace. Once restored,
// the team_member role can be added to a user allowing the database to be
// inspected or maintained using the frontend.
// If uploading a new file, remember to copy the links into the tests fileShare
// folder as well.

#define TESTDB "ClashDetection"
#define TESTDBTMP "ClashDetectionTmp"

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


std::vector<repo::core::model::RepoBSON> ClashDetectionDatabaseHelper::getChildMeshNodes(repo::lib::Container* container, std::string name)
{
	auto uuids = getUniqueIdsByName(container, name);
	return handler->findAllByCriteria(container->teamspace,
		container->container + "." + REPO_COLLECTION_SCENE,
		repo::core::handler::database::query::Eq(REPO_NODE_LABEL_ID, uuids),
		true
	);
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
		REPO_COLLECTION_SETTINGS,
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
	std::vector<repo::lib::RepoUUID> parentSharedIds;

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

		for (auto& bson : *cursor) {
			auto p = bson.getUUIDFieldArray(REPO_NODE_LABEL_PARENTS);
			parentSharedIds.insert(
				parentSharedIds.end(),
				p.begin(),
				p.end()
			);
		}
	}

	// By convention, all metadata nodes should wire to both the parent transform,
	// but also any mesh descendents.

	// This method assumes that the import settings are such that each element has
	// only one mesh. This is for performance reasons, as otherwise it would
	// require nested queries which can be very expensive. If this is the case,
	// another method will need to be used.

	{
		repo::core::handler::database::query::RepoQueryBuilder query;
		query.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_SHARED_ID, parentSharedIds));
		query.append(repo::core::handler::database::query::Eq(REPO_NODE_LABEL_TYPE, std::string(REPO_NODE_TYPE_MESH)));

		repo::core::handler::database::query::RepoProjectionBuilder projection;
		projection.includeField(REPO_NODE_LABEL_ID);

		auto cursor = handler->findCursorByCriteria(
			container->teamspace,
			container->container + "." + REPO_COLLECTION_SCENE,
			query,
			projection
		);

		for (auto& bson : *cursor) {
			auto uuid = bson.getUUIDField(REPO_NODE_LABEL_ID);
			CompositeObject composite;
			composite.id = repo::lib::RepoUUID::createUUID();
			composite.meshes.push_back(MeshReference(container, uuid));
			objects.push_back(composite);
		}
	}
}

std::unique_ptr<repo::lib::Container> testing::makeTemporaryContainer()
{
	auto container = std::make_unique<repo::lib::Container>();
	container->container = repo::lib::RepoUUID::createUUID().toString();
	container->revision = repo::lib::RepoUUID::createUUID();
	container->teamspace = TESTDBTMP;
	return container;
}

void testing::importModel(std::string filename, const repo::lib::Container& container)
{
	using namespace repo::manipulator::modelutility;
	using namespace repo::manipulator::modelconvertor;

	ModelImportConfig config(
		container.revision,
		container.teamspace,
		container.container
	);
	config.targetUnits = ModelUnits::MILLIMETRES;

	auto handler = getHandler();

	uint8_t err;
	std::string msg;

	ModelImportManager manager;
	auto scene = manager.ImportFromFile(filename, config, handler, err);
	scene->commit(handler.get(), handler->getFileManager().get(), msg, "testuser", "", "", config.getRevisionId());

	// Clash detection expects the units to be stored in model settings
	handler->upsertDocument(
		container.teamspace,
		REPO_COLLECTION_SETTINGS,
		testing::makeProjectSettings(container.container),
		false
	);

	delete scene;
}

repo::core::model::MeshNode testing::createPointMesh(std::initializer_list<repo::lib::RepoVector3D> points)
{
	std::vector<repo::lib::repo_face_t> faces;
	for (size_t i = 0; i < points.size(); ++i)
	{
		faces.push_back({ i });
	}
	return repo::core::model::RepoBSONFactory::makeMeshNode(
		std::vector<repo::lib::RepoVector3D>(points),
		faces,
		{},
		{}
	);
}

bool testing::intersects(const std::vector<repo::lib::RepoTriangle>& a, const std::vector<repo::lib::RepoTriangle>& b)
{
	for (const auto& t1 : a) {
		for (const auto& t2 : b) {
			if (geometry::intersects(t1, t2) > geometry::COPLANAR) {
				return true;
			}
		}
	}
	return false;
}

ClearanceAccuracyReport::ClearanceAccuracyReport(std::string filename) {
	file.open(filename, std::ios::out | std::ios::binary);
}

void ClearanceAccuracyReport::add(const ClashDetectionReport& report, double nominalDistance)
{
	for (auto& clash : report.clashes) {
		auto e = abs((clash.positions[0] - clash.positions[1]).norm() - nominalDistance);
		file.write(reinterpret_cast<const char*>(&e), sizeof(double));
	}
}

ClearanceAccuracyReport::~ClearanceAccuracyReport() {
	file.close();
}