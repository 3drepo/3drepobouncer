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

#pragma once

#include <repo/lib/datastructure/repo_vector.h>
#include <repo/lib/datastructure/repo_container.h>
#include <repo/lib/datastructure/repo_triangle.h>
#include <repo/lib/datastructure/repo_structs.h>
#include <repo/manipulator/modelutility/repo_clash_detection_engine.h>
#include <repo/manipulator/modelutility/repo_clash_detection_config.h>
#include <repo/manipulator/modelutility/clashdetection/geometry_utils.h>
#include <repo/core/handler/repo_database_handler_mongo.h>
#include <repo/core/model/bson/repo_node_mesh.h>

#include <vector>
#include <set>

namespace testing {

	using DatabasePtr = std::shared_ptr<repo::core::handler::MongoDatabaseHandler>;

	struct LinkFileEntry {
		std::string clashSetName;
		double parameterValue;
	};

	// Helper class to build clash detection configurations using the user-fiendly
	// identifiers of Nodes in the Clash Detection database, such as their names.

	struct ClashDetectionDatabaseHelper
	{
		ClashDetectionDatabaseHelper(DatabasePtr handler)
			:handler(handler)
		{
		}

		DatabasePtr handler;

		void getChildMeshNodes(repo::lib::Container* container, 
			const repo::core::model::RepoBSON& bson, 
			std::set<repo::lib::RepoUUID>& uuids);

		geometry::RepoIndexedMesh getChildMeshNodes(
			repo::lib::Container* container,
			const std::string name);

		// Searches for mesh nodes only

		std::set<repo::lib::RepoUUID> getUniqueIdsByName(
			repo::lib::Container* container,
			std::string name);

		void GetMetadataMap(
			repo::lib::Container* container,
			std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher>& metadataMap);

		void SplitIntoCompositSetsByLinkfileData(
			std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher>& metadataMap,
			std::unordered_map<std::string, LinkFileEntry>& linkFileEntries,
			std::set<repo::lib::RepoUUID>& sharedIdsA,
			std::set<repo::lib::RepoUUID>& sharedIdsB
		);

		void SplitIntoCompositSetsByLinkfileData(
			std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher>& metadataMap,
			std::unordered_map<std::string, LinkFileEntry>& linkFileEntries,
			std::set<repo::lib::RepoUUID>& sharedIdsA,
			std::set<repo::lib::RepoUUID>& sharedIdsB,
			std::unordered_map<repo::lib::RepoUUID, double, repo::lib::RepoUUIDHasher>& parameterMap
		);

		void SplitIntoCompositSetsByMetadata(
			std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher>& metadataMap,
			std::set<repo::lib::RepoUUID>& sharedIdsA,
			std::set<repo::lib::RepoUUID>& sharedIdsB,
			std::string nameKey,
			std::string nameA,
			std::string nameB
		);

		void SplitIntoCompositSetsByMetadata(
			std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher>& metadataMap,
			std::set<repo::lib::RepoUUID>& sharedIdsA,
			std::set<repo::lib::RepoUUID>& sharedIdsB,
			std::unordered_map<repo::lib::RepoUUID, double, repo::lib::RepoUUIDHasher>& parameterMap,
			std::string nameKey,
			std::string nameA,
			std::string nameB,
			std::string parameterKey
		);

		void setCompositeObjectsBySharedIds(
			repo::manipulator::modelutility::ClashDetectionConfig& config,
			const std::unique_ptr<repo::lib::Container>& container,
			const std::set<repo::lib::RepoUUID>& sharedIdsA,
			const std::set<repo::lib::RepoUUID>& sharedIdsB);

		void setCompositeObjectsBySharedIds(
			repo::manipulator::modelutility::ClashDetectionConfig& config,
			const std::unique_ptr<repo::lib::Container>& container,
			const std::set<repo::lib::RepoUUID>& sharedIdsA,
			const std::set<repo::lib::RepoUUID>& sharedIdsB,
			std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>& compToSharedId);

		void createCompositeObjectsBySharedIds(
			std::vector<repo::manipulator::modelutility::CompositeObject>& objects,
			const std::unique_ptr<repo::lib::Container>& container,
			const std::set<repo::lib::RepoUUID>& sharedIds);

		void createCompositeObjectsBySharedIds(
			std::vector<repo::manipulator::modelutility::CompositeObject>& objects,
			const std::unique_ptr<repo::lib::Container>& container,
			const std::set<repo::lib::RepoUUID>& sharedIds,
			std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoUUID, repo::lib::RepoUUIDHasher>& compToSharedId);

		repo::manipulator::modelutility::CompositeObject createCompositeObject(
			repo::lib::Container* container,
			std::string name
		);

		// Will replace anything in the existing sets

		void setCompositeObjectSetsByName(
			repo::manipulator::modelutility::ClashDetectionConfig& config,
			const std::unique_ptr<repo::lib::Container>& container,
			std::initializer_list<std::string> a,
			std::initializer_list<std::string> b);

		// Sets the members of the sets by the existance of a metadata node with the
		// given value. This method does not care to which key the value belons,
		// so ensure it is sufficiently unique. Will replace the existing sets.

		void setCompositeObjectsByMetadataValue(
			repo::manipulator::modelutility::ClashDetectionConfig& config,
			const std::unique_ptr<repo::lib::Container>& container,
			const std::string& valueSetA,
			const std::string& valueSetB);

		void setCompositeObjectsByMetadataValue(
			repo::manipulator::modelutility::ClashDetectionConfig& config,
			const std::unique_ptr<repo::lib::Container>& container,
			const std::string& valueSetA,
			const std::string& valueSetB,
			std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher>& metadataMap);

		void createCompositeObjectsFromContainer(
			std::vector<repo::manipulator::modelutility::CompositeObject>& objects,
			repo::lib::Container* container);

		void createCompositeObjectsByMetadataValue(
			std::vector<repo::manipulator::modelutility::CompositeObject>& objects,
			repo::lib::Container* container,
			const std::string& value);

		void createCompositeObjectsByMetadataValue(
			std::vector<repo::manipulator::modelutility::CompositeObject>& objects,
			repo::lib::Container* container,
			const std::string& value,
			std::unordered_map<repo::lib::RepoUUID, std::unordered_map<std::string, repo::lib::RepoVariant>, repo::lib::RepoUUIDHasher>& metadataMap);

		void getBoundsForContainer(
			repo::lib::Container* container,
			std::unordered_map<repo::lib::RepoUUID, repo::lib::RepoBounds, repo::lib::RepoUUIDHasher>& boundsMap);

		std::unique_ptr<repo::lib::Container> getContainerByName(
			std::string name);
	};

	repo::manipulator::modelutility::CompositeObject combineCompositeObjects(
		std::initializer_list<repo::manipulator::modelutility::CompositeObject> objects
	);

	/*
	* This class is used to collect the outputs from the clash detection engine and
	* from them collect the measured distances, writing the errors when compared
	* with a ground truth to disk.
	*
	* The purpose is to generate a set of samples that describe the error distribution
	* of the engine, that can be analysed later in another tool.
	*
	* This class is used by AccuracyReport test, which is disabled by default. Comment
	* out the skip macro and run that test in isolation to generate a new report.
	*/
	class ClearanceAccuracyReport
	{
		std::fstream file;

	public:
		ClearanceAccuracyReport(std::string filename);
		void add(const repo::manipulator::modelutility::ClashDetectionReport& report, double nominalDistance);
		~ClearanceAccuracyReport();
	};

	/*
	* Creates a Container populated with suitabily unique uuids for importing
	* into the Temporary Clash Test Database. (This does not create the container
	* in the database - just the description.)
	*/
	std::unique_ptr<repo::lib::Container> makeTemporaryContainer();

	/*
	* Imports a model into the given Container. This only imports the scene,
	* it does not generate any assets such as RepoBundles.
	*/
	void importModel(std::string filename, const repo::lib::Container& container);

	repo::core::model::MeshNode createPointMesh(std::initializer_list<repo::lib::RepoVector3D> points);

	bool intersects(const std::vector<repo::lib::RepoTriangle>& a, const std::vector<repo::lib::RepoTriangle>& b);
}