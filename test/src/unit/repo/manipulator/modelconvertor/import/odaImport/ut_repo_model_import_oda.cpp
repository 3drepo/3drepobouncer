/**
*  Copyright (C) 2020 3D Repo Ltd
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>
#include <repo/manipulator/modelconvertor/import/repo_model_import_oda.h>
#include <repo/lib/repo_log.h>
#include "../../../../../repo_test_utils.h"
#include "../../../../../repo_test_database_info.h"
#include "boost/filesystem.hpp"
#include "../../bouncer/src/repo/error_codes.h"

using namespace repo::manipulator::modelconvertor;
using namespace repo::core::model;
using namespace testing;

#define DBNAME "ODAModelImport"

#pragma optimize("",off)

/* ODA is predominantly tested via the system tests using the client. These tests
 * cover specific features or regressions. */

inline void hash_combine(std::size_t& seed) { }

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	hash_combine(seed, rest...);
}

// Define some fingerprints of common types so we can use std::hash with them
// in the following tests

namespace std
{
	template<> struct hash<repo::lib::RepoMatrix> {
		size_t operator()(const repo::lib::RepoMatrix& n) const {
			size_t hash = 0;
			for (auto f : n.getData()) {
				hash_combine(hash, std::hash<float>{}(f));
			}
			return hash;
		}
	};

	template<> struct hash<repo::lib::RepoBounds> {
		size_t operator()(const repo::lib::RepoBounds& n) const {
			size_t hash = 0;
			hash_combine(hash, std::hash<double>{}(n.min().x), std::hash<double>{}(n.min().y), std::hash<double>{}(n.min().z));
			hash_combine(hash, std::hash<double>{}(n.max().x), std::hash<double>{}(n.max().y), std::hash<double>{}(n.max().z));
			return hash;
		}
	};

	template<> struct hash<TransformationNode> {
		size_t operator()(const TransformationNode& n) const {
			size_t hash = 0;
			hash_combine(hash,
				std::hash<std::string>{}(n.getName()),
				std::hash<repo::lib::RepoMatrix>{}(n.getTransMatrix())
			);
			return hash;
		}
	};

	template<> struct hash<MeshNode> {
		size_t operator()(const MeshNode& n) const {
			size_t hash = 0;
			hash_combine(hash,
				std::hash<std::string>{}(n.getName()),
				std::hash<int>{}(n.getNumFaces()),
				std::hash<int>{}(n.getNumUVChannels()),
				std::hash<int>{}(n.getNumVertices()),
				std::hash<repo::lib::RepoBounds>{}(n.getBoundingBox()),
				std::hash<int>{}((int)n.getPrimitive()),
				std::hash<std::string>{}(n.getGrouping())
			);
			return hash;
		}
	};

	template<> struct hash<MetadataNode> {

		struct Options
		{
			// When true, the actual metadata will be ignored and the hash is based on the node name.
			bool nameOnly = false;

			// When true, the metadata is first sorted by its key values before the hash is built.
			bool sortFirst = true;
		};

		Options opts;

		size_t operator()(const MetadataNode& n) const {
			size_t hash = 0;
			hash_combine(hash, std::hash<std::string>{}(n.getName()));
			if (!opts.nameOnly)
			{

			}
			return hash;
		}
	};
}

namespace ODAModelImportUtils
{
	std::unique_ptr<AbstractModelImport> ImportFile(
		std::string filePath,
		uint8_t& impModelErrCode)
	{
		ModelImportConfig config;
		auto modelConvertor = std::unique_ptr<AbstractModelImport>(new OdaModelImport(config));
		modelConvertor->importModel(filePath, impModelErrCode);
		return modelConvertor;
	}

	/*
	* This class compares two scene graphs to check if they have the same content.
	* The class considers only deterministic fields, such as names and geometry,
	* but is independent of non-deterministic fields such as UUIDs or timestamps.
	* This class assumes that the scene collection contains only one revision.
	* 
	* The algorithm to compare scenes attempts to match nodes between the two trees.
	* 
	* Take care that the reported differences between nodes may be due to a mismatch
	* between the two trees: if a property change causes the relative position of
	* nodes to change with respect to the other tree, then the comparison may
	* encounter these differences first. This is most likely to occur when comparing
	* meshes with no names.
	*/
	class SceneComparer
	{
	public:
		SceneComparer()
		{
			handler = getHandler();
		}

		/*
		* The allowed deviation between any of the six components when comparing two
		* bounding boxes.
		*/
		double boundingBoxTolerance = 0.000001;


		bool ignoreMetadataContent = false;

		struct Node
		{
			std::shared_ptr<RepoNode> dbNode;

			// The hash of this node, used to control the order relative to the node's
			// siblings.
			size_t hash;

			// This should be ordered by the time the scene is fully loaded.
			std::vector<Node*> children;

			size_t combinedHash() const
			{
				size_t h = hash;
				for (auto& c : children) {
					hash_combine(h, c->hash);
				}
				return h;
			}
		};

		// Compares the database node types of two Nodes, intended to be used
		// for sorting. If one of the nodes matches the type, return the result
		// of the comparison type A > type B. If neither matches, or both types
		// are the same, returns 0.

		template <typename T>
		static int compareNodeTypes(const Node* a, const Node* b)
		{
			if (std::dynamic_pointer_cast<T>(a->dbNode) && !std::dynamic_pointer_cast<T>(b->dbNode))
			{
				return 1;
			}
			else if (std::dynamic_pointer_cast<T>(b->dbNode) && !std::dynamic_pointer_cast<T>(a->dbNode))
			{
				return -1;
			}
			return 0;
		}

		struct Scene
		{
			void addNode(std::shared_ptr<TransformationNode> n)
			{
				addNode(n, std::hash<TransformationNode>{}(*n));
			}

			void addNode(std::shared_ptr<MeshNode> n)
			{
				addNode(n, std::hash<MeshNode>{}(*n));
			}

			void addNode(std::shared_ptr<MetadataNode> n)
			{
				addNode(n, std::hash<MetadataNode>{}(*n));
			}

			void addNode(std::shared_ptr<RepoNode> dbNode, size_t hash)
			{
				Node node;
				node.dbNode = dbNode;
				node.hash = hash;
				nodes.push_back(node);
			}

			// Populate the children vector of each outer node, using the parentIds member
			// of the RepoNodes. Updates to the shared Id map, child and root pointers
			// should happen once, only after nodes has been fully initialised, or else
			// the underlying memory may change.

			void populateChildNodes()
			{
				std::unordered_map<repo::lib::RepoUUID, Node*, repo::lib::RepoUUIDHasher> sharedIdToNode;

				for (auto& n : nodes)
				{
					sharedIdToNode[n.dbNode->getSharedID()] = &n;
					if (n.dbNode->getName() == "rootNode" && n.dbNode->getParentIDs().empty())
					{
						root = &n;
					}
				}

				for (auto& n : nodes)
				{
					for (auto p : n.dbNode->getParentIDs())
					{
						sharedIdToNode[p]->children.push_back(&n);
					}
				}
			}

			// Nodes may be returned from the database in any order. To ensure we can
			// match between two sets of siblings, nodes are given a hash based on their
			// deterministic fields, which is used to order them. The order should end up
			// the same between two identical scenes.
			// 
			// If siblings have identical hashes, then their hashes are combined with
			// those of their children to disambiguate them.

			void orderChildNodes()
			{
				orderChildNodes(root);
			}

			void orderChildNodes(Node* node)
			{
				auto& children = node->children;

				// Combining the hash of a node with its children is sensitive to the child
				// order, so it is important that any refinement is performed bottom-up,
				// which is achieved by sorting recursively.

				for (auto& c : children) {
					orderChildNodes(c);
				}

				// Nodes are sorted by different characteristics in a descending priority.
				// If two siblings are indistinguishable based on their own content, the
				// final disambiguation is to combine their has with their child nodes. The
				// child nodes should already be ordered by this point, meaning if the two
				// nodes remain indistinguishable, then its likely both entire branches are
				// equivalent (and local order does not matter).

				struct Comparer
				{
					bool operator()(const Node* a, const Node* b) const
					{
						// First order by type (meta -> mesh -> transformation)

						auto cmp = compareNodeTypes<MetadataNode>(a, b);
						if (cmp) {
							return cmp > 0;
						}

						cmp = compareNodeTypes<MeshNode>(a, b);
						if (cmp) {
							return cmp > 0;
						}

						cmp = compareNodeTypes<TransformationNode>(a, b);
						if (cmp) {
							return cmp > 0;
						}

						// Then by name

						if (a->dbNode->getName() != b->dbNode->getName())
						{
							return a->dbNode->getName() < b->dbNode->getName();
						}

						// If the names match, then by hash

						if (a->hash != b->hash)
						{
							return a->hash < b->hash;
						}

						// If the hashes match, finally by the combined hashes the child nodes
						// (which should already be in order)

						return a->combinedHash() < b->combinedHash();
					}
				};

				Comparer comparer;
				std::sort(children.begin(), children.end(), comparer);
			}

			// This is the master list of nodes in the scene. Once the children members
			// are initialised this should not change, as it may move the memory around
			// and break those pointers.
			std::vector<Node> nodes;

			Node* root;
		};

		struct Result
		{
			std::string message;

			// Conversion operator that says whether the comparision has succeeded or not
			operator bool () const
			{
				return message.empty();
			}
		};

		Result compare(std::string expectedDb, std::string expectedName, std::string actualDb, std::string actualName)
		{
			Result result;
			auto expected = loadScene(expectedDb, expectedName);
			auto actual = loadScene(actualDb, actualName);
			try 
			{
				compare({ expected->root, actual->root });
			}
			catch (ComparisonException e)
			{
				result.message = e.explain();
			}
			return result;
		}

	private:

		using Pair = std::tuple<Node*, Node*>;

		class ComparisonException : public std::exception
		{
			std::string message;

		public:
			ComparisonException(const char* description)
			{
				message = std::string(description);
			}

			ComparisonException(std::shared_ptr<RepoNode> expected, std::shared_ptr<RepoNode> actual, const char* description)
			{
				message = expected->getUniqueID().toString() + " != " + actual->getUniqueID().toString() + " " + std::string(description);
			}

			std::string explain()
			{
				return message;
			}
		};

		/*
		* Loads all the scene nodes into the Scene object and computes their hashes.
		* We don't consider mesh materials in this version.
		*/
		std::shared_ptr<Scene> loadScene(std::string db, std::string name)
		{
			auto scene = std::make_shared<Scene>();
			auto bsons = handler->getAllFromCollectionTailable(db, name + "." + REPO_COLLECTION_SCENE);
			for (auto& bson : bsons)
			{
				auto type = bson.getStringField(REPO_LABEL_TYPE);
				if (type == REPO_NODE_TYPE_TRANSFORMATION)
				{
					scene->addNode(std::make_shared<TransformationNode>(bson));
				}
				else if (type == REPO_NODE_TYPE_MESH)
				{
					scene->addNode(std::make_shared<MeshNode>(bson));
				}
				else if (type == REPO_NODE_TYPE_METADATA)
				{
					scene->addNode(std::make_shared<MetadataNode>(bson));
				}
			}

			scene->populateChildNodes();
			scene->orderChildNodes();

			return scene;
		}

		void compare(Pair begin)
		{
			std::stack<Pair> nodesToCompare;
			nodesToCompare.push(begin);

			while (!nodesToCompare.empty())
			{
				auto pair = nodesToCompare.top();
				nodesToCompare.pop();

				auto expected = std::get<0>(pair);
				auto actual = std::get<1>(pair);

				// This compares the node fields themselves semantically
				compare(expected->dbNode, actual->dbNode);

				if (expected->children.size() != actual->children.size())
				{
					throw ComparisonException(expected->dbNode, actual->dbNode, "Different numbers of children");
				}

				for (int i = 0; i < expected->children.size(); i++)
				{
					nodesToCompare.push({ expected->children[i], actual->children[i] });
				}
			}
		}

		void compare(std::shared_ptr<RepoNode> expected, std::shared_ptr<RepoNode> actual)
		{
			if (typeid(*expected) != typeid(*actual))
			{
				throw ComparisonException(expected, actual, "Are different types");
			}

			if (expected->getName() != actual->getName())
			{
				throw ComparisonException(expected, actual, "Names do not match");
			}

			if (std::dynamic_pointer_cast<TransformationNode>(expected))
			{
				compareTransformationNode(std::dynamic_pointer_cast<TransformationNode>(expected), std::dynamic_pointer_cast<TransformationNode>(actual));
			}
			else if (std::dynamic_pointer_cast<MeshNode>(expected))
			{
				compareMeshNode(std::dynamic_pointer_cast<MeshNode>(expected), std::dynamic_pointer_cast<MeshNode>(actual));
			}
			else if (std::dynamic_pointer_cast<MetadataNode>(expected))
			{
				compareMetaNode(std::dynamic_pointer_cast<MetadataNode>(expected), std::dynamic_pointer_cast<MetadataNode>(actual));
			}
		}

		bool compare(const repo::lib::RepoBounds& a, const repo::lib::RepoBounds& b)
		{
			auto dmin = a.min() - b.min();
			auto dmax = a.max() - b.max();
			return !(
				abs(dmin.x) > boundingBoxTolerance ||
				abs(dmin.y) > boundingBoxTolerance ||
				abs(dmin.z) > boundingBoxTolerance ||
				abs(dmax.x) > boundingBoxTolerance ||
				abs(dmax.y) > boundingBoxTolerance ||
				abs(dmax.z) > boundingBoxTolerance
			);
		}

		void compareTransformationNode(std::shared_ptr<TransformationNode> expected, std::shared_ptr<TransformationNode> actual)
		{
			if (expected->getTransMatrix() != actual->getTransMatrix())
			{
				throw ComparisonException(expected, actual, "Matrices to not match");
			}
		}

		void compareMeshNode(std::shared_ptr<MeshNode> expected, std::shared_ptr<MeshNode> actual)
		{
			if (expected->getGrouping() != actual->getGrouping())
			{
				throw ComparisonException(expected, actual, "Grouping does not match");
			}

			if (expected->getPrimitive() != actual->getPrimitive())
			{
				throw ComparisonException(expected, actual, "Primitive does not match");
			}

			if (!compare(expected->getBoundingBox(), actual->getBoundingBox()))
			{
				throw ComparisonException(expected, actual, "Bounding box does not match");
			}

			if (expected->getFaces() != actual->getFaces())
			{
				throw ComparisonException(expected, actual, "Faces do not match");
			}

			if (expected->getVertices() != actual->getVertices())
			{
				throw ComparisonException(expected, actual, "Vertices do not match");
			}

			if (expected->getNormals() != actual->getNormals())
			{
				throw ComparisonException(expected, actual, "Normals do not match");
			}

			if (expected->getUVChannelsSerialised() != actual->getUVChannelsSerialised())
			{
				throw ComparisonException(expected, actual, "UV channels do not match");
			}
		}

		void compareMetaNode(std::shared_ptr<MetadataNode> expected, std::shared_ptr<MetadataNode> actual)
		{
			if (!ignoreMetadataContent)
			{
				throw std::exception("Not yet implemented");
			}
		}
		
		std::shared_ptr<repo::core::handler::MongoDatabaseHandler> handler;
	};
}

MATCHER(IsSuccess, "")
{
	*result_listener << arg.message;
	return (bool)arg;
}

TEST(ODAModelImport, CheckReferenceDatabases)
{
	// Checks for the existence of the SampleNWDTree reference database.
	bool haveImported = false;

	if (!projectExists(DBNAME, "Sample2025NWDODA"))
	{
		uint8_t errCode = 0;
		auto importer = ODAModelImportUtils::ImportFile(getDataPath("sample2025.nwd"), errCode);
		auto scene = importer->generateRepoScene(errCode);
		auto handler = getHandler();
		std::string errMsg;
		scene->setDatabaseAndProjectName(DBNAME, "Sample2025NWDODA");
		scene->commit(handler.get(), handler->getFileManager().get(), errMsg, "unit tests", "", "");
	}

	if (!projectExists(DBNAME, "Sample2025NWD")) {
		runProcess(produceUploadArgs(DBNAME, "Sample2025NWD", getDataPath("sample2025.nwd")));
		haveImported = true;
	}

	if (!projectExists(DBNAME, "Sample2025NWD2")) {
		runProcess(produceUploadArgs(DBNAME, "Sample2025NWD2", getDataPath("sample2025.nwd")));
		haveImported = true;
	}

	if(haveImported)
	{
		FAIL() << "One or more reference databases were not found. These have been reinitialised but must be checked manually and committed.";
	}
}

TEST(ODAModelImport, Sample2025NWD)
{
	uint8_t errCode = 0;
	/*
	auto importer = ODAModelImportUtils::ImportFile(getDataPath("sample2025.nwd"), errCode);
	auto scene = importer->generateRepoScene(errCode);
	scene->setDatabaseAndProjectName("ODAModelImportTest", "Sample2025NWDODA");
	auto handler = getHandler();
	std::string errMsg;
	scene->commit(handler.get(), handler->getFileManager().get(), errMsg, "unit tests", "", "");
	*/

	ODAModelImportUtils::SceneComparer comparer;
	comparer.ignoreMetadataContent;

	//EXPECT_THAT(comparer.compare(DBNAME, "Sample2025NWD", "ODAModelImport", "Sample2025NWD2"), IsSuccess());
	EXPECT_THAT(comparer.compare(DBNAME, "Sample2025NWDODA", "ODAModelImportTest", "Sample2025NWDODA"), IsSuccess());
}

// todo::

// Variations in metadata value
// Variations in geometry