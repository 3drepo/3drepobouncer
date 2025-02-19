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
#include "repo/core/model/bson/repo_bson_factory.h"
#include "repo/core/handler/database/repo_query.h"

#include <thread>
#include <variant>
#include <semaphore>
#include <thread>
#include "spscqueue/readerwriterqueue.h"

using namespace repo::manipulator::modelutility;
using namespace repo::core::model;

// 500 Mb
#define DEFAULT_THRESHOLD 1024*1024*500

/*
* The async worker of RepoSceneBuilder is responsible for the multithreaded
* writes. It's public API is expected to be called from the same thread as
* RepoSceneBuilder, where it can exert backpressure by blocking. Internally it
* maintains a worker that holds a database bulk write context.
* Destroying the object will block until the bulk write context is finished.
* AsyncImpl is designed to be cheap to create - to flush it, just destroy it,
* and make another one if necessary.
*/
class RepoSceneBuilder::AsyncImpl
{
public:
	AsyncImpl(RepoSceneBuilder* builder);

	~AsyncImpl();

	size_t threshold;

	/*
	* Adds an item to the queue for processing. This method will block until there
	* is space in the queue. Passing the pointer transfers ownership to the queue.
	*/
	void push(repo::core::model::RepoNode* node);
	void push(repo::core::handler::database::query::AddParent*);

private:
	/*
	* The implementation of the consumer uses a visitor that belongs to the worker
	* thread. This works well because the number of entities we wish to pass are
	* few and small, and the SPSC queue used allows passing the variants by value.
	*
	* An alternative implementation is to pass pointers to functors. The advantage
	* of this is that the consumer function is trivial. The downside is that we'd
	* need to allocate a wrapper for entities that are already simple pointers, but
	* it is worth keeping in mind if either of the above change.
	*/

	struct Close { // Passing this object instructs the worker to flush and terminate
	};

	struct Notify { // Passing this object instructs the worker to release the semaphore
	};

	using Consumables = std::variant<
		repo::core::model::RepoNode*,
		repo::core::handler::database::query::AddParent*,
		Close,
		Notify
	>;

	struct Consumable {
		Consumables object;
		size_t size;
	};

	void push(Consumable consumable);

	struct Consumer
	{
		Consumer(RepoSceneBuilder::AsyncImpl* impl);

		std::unique_ptr<repo::core::handler::database::BulkWriteContext> collection;
		RepoSceneBuilder::AsyncImpl* impl;

		bool operator() (repo::core::model::RepoNode* n) const;
		bool operator() (const repo::core::handler::database::query::AddParent* n) const;
		bool operator() (const  Close& n) const;
		bool operator() (const  Notify& n) const;
	};

	/* This will run as a member function */
	void consumerFunction();

	RepoSceneBuilder* builder;

	moodycamel::BlockingReaderWriterQueue<Consumable> queue;
	std::thread consumer;

	/*
	* The size of the queue - this is a unitless value that is compared with a
	* fixed threshold. In practice, currently it represents the estimated memory
	* usage of each node or update object.
	*/
	std::atomic<size_t> queueSize;

	/*
	* When the amount of data buffered exceeds the threshold, the main thread
	* will attempt to acquire the semaphore to suspend itself until signalled
	* by the consumer.
	*/
	std::binary_semaphore block;

	/*
	* If the consumer thread throws an exception, this will hold it. By default it
	* is checked and rethrown in the destructor.
	*/
	std::exception_ptr consumerException;
};

struct RepoSceneBuilder::Deleter
{
	RepoSceneBuilder* builder;
	void operator()(RepoNode* n)
	{
		builder->addNode(std::unique_ptr<RepoNode>(n));
		builder->referenceCounter--;
	}
};

RepoSceneBuilder::RepoSceneBuilder(
	std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler,
	const std::string& database,
	const std::string& project,
	const repo::lib::RepoUUID& revisionId)
	:handler(handler),
	databaseName(database),
	projectName(project),
	revisionId(revisionId),
	referenceCounter(0),
	nodeCount(0),
	isMissingTextures(false),
	offset({}),
	units(repo::manipulator::modelconvertor::ModelUnits::UNKNOWN),
	impl(std::make_unique<AsyncImpl>(this))
{
}

RepoSceneBuilder::~RepoSceneBuilder()
{
	if (referenceCounter)
	{
		throw std::runtime_error("RepoSceneBuilder is being destroyed with outstanding RepoNode references. Make sure all RepoNodes have gone out of scope before RepoSceneBuilder.");
	}

	if (parentUpdates.size())
	{
		throw std::runtime_error("RepoSceneBuilder is being destroyed with outstanding updates. Make sure to call finalise before letting RepoSceneBuilder go out of scope.");
	}
}

template<repo::core::model::RepoNodeClass T>
std::shared_ptr<T> RepoSceneBuilder::addNode(const T& node)
{
	Deleter deleter;
	deleter.builder = this;
	auto ptr = std::shared_ptr<T>(new T(node), deleter);
	ptr->setRevision(revisionId);
	referenceCounter++;
	return ptr;
}

void RepoSceneBuilder::addNode(std::unique_ptr<repo::core::model::RepoNode> node)
{
	node->setRevision(revisionId);
	queueNode(node.release());
}

void RepoSceneBuilder::addNodes(std::vector<std::unique_ptr<repo::core::model::RepoNode>> nodes)
{
	for (auto& n : nodes) {
		addNode(std::move(n));
	}
}

void RepoSceneBuilder::queueNode(RepoNode* node)
{
	nodeCount++;
	impl->push(node);

	if (nodeCount % 5000 == 0) {
		repoInfo << "Committed " << nodeCount << " nodes...";
	}
}

std::string RepoSceneBuilder::getSceneCollectionName()
{
	return projectName + "." + REPO_COLLECTION_SCENE;
}

void RepoSceneBuilder::commit()
{
	for (auto u : parentUpdates) {
		impl->push(u.second);
	}
	parentUpdates.clear();
}

void RepoSceneBuilder::finalise()
{
	commit();
	impl = std::make_unique<AsyncImpl>(this); // Destroying the AsyncImpl will flush everything to the database
}

repo::lib::RepoVector3D64 RepoSceneBuilder::getWorldOffset()
{
	return offset;
}

void RepoSceneBuilder::setWorldOffset(const repo::lib::RepoVector3D64& offset)
{
	this->offset = offset;
}

void RepoSceneBuilder::addMaterialReference(const repo::lib::repo_material_t& m, repo::lib::RepoUUID parentId)
{
	auto key = m.checksum();
	if (materialToUniqueId.find(key) == materialToUniqueId.end())
	{
		auto node = repo::core::model::RepoBSONFactory::makeMaterialNode(m, {}, { parentId });
		addNode(node);
		materialToUniqueId[key] = node.getUniqueID();

		if (m.hasTexture()) {
			addTextureReference(m.texturePath, node.getSharedID());
		}
	}
	else
	{
		addParent(materialToUniqueId[key], parentId);
	}
}

void RepoSceneBuilder::addTextureReference(std::string texture, repo::lib::RepoUUID parentId)
{
	if (textureToUniqueId.find(texture) == textureToUniqueId.end()) {
		auto node = createTextureNode(texture);
		if (node) {
			node->addParent(parentId);
			textureToUniqueId[texture] = node->getUniqueID();
			addNode(std::move(node));
		}
	}
	else
	{
		addParent(textureToUniqueId[texture], parentId);
	}
}

std::unique_ptr<repo::core::model::TextureNode> RepoSceneBuilder::createTextureNode(const std::string& texturePath)
{
	std::unique_ptr<repo::core::model::TextureNode> node;
	std::ifstream::pos_type size;
	std::ifstream file(texturePath, std::ios::in | std::ios::binary | std::ios::ate);
	char* memblock = nullptr;
	if (!file.is_open())
	{
		setMissingTextures();
		return node;
	}

	size = file.tellg();
	memblock = new char[size];
	file.seekg(0, std::ios::beg);
	file.read(memblock, size);
	file.close();

	node = std::make_unique<repo::core::model::TextureNode>(repo::core::model::RepoBSONFactory::makeTextureNode(
		texturePath,
		(const char*)memblock,
		size,
		1,
		0
	));

	delete[] memblock;

	return node;
}

void RepoSceneBuilder::addParent(repo::lib::RepoUUID nodeUniqueId, repo::lib::RepoUUID parentSharedId)
{
	// Inheritence changes have very small footprints, so keep a few thousand of
	// these on hand in case we get lots of references to the same entity, and so
	// can perform back writes to a single document and so save on the number of
	// individual database writes.

	using namespace repo::core::handler::database;

	if (parentUpdates.find(nodeUniqueId) != parentUpdates.end())
	{
		parentUpdates[nodeUniqueId]->parentIds.push_back(parentSharedId);
	}
	else
	{
		parentUpdates[nodeUniqueId] = new query::AddParent(nodeUniqueId, parentSharedId);
	}

	if (parentUpdates.size() > 10000)
	{
		commit();
	}
}

void RepoSceneBuilder::setMissingTextures()
{
	this->isMissingTextures = true;
}

bool RepoSceneBuilder::hasMissingTextures()
{
	return isMissingTextures;
}

void RepoSceneBuilder::setUnits(repo::manipulator::modelconvertor::ModelUnits units) 
{
	this->units = units;
}

repo::manipulator::modelconvertor::ModelUnits RepoSceneBuilder::getUnits()
{
	return this->units;
}

void RepoSceneBuilder::createIndexes()
{
	using namespace repo::core::handler::database::index;
	handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_HISTORY, Descending({ REPO_NODE_REVISION_LABEL_TIMESTAMP }));
	handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_SCENE, Ascending({ REPO_NODE_REVISION_ID, "metadata.key", "metadata.value" }));
	handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_SCENE, Ascending({ "metadata.key", "metadata.value" }));
	handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_SCENE, Ascending({ REPO_NODE_REVISION_ID, REPO_NODE_LABEL_SHARED_ID, REPO_LABEL_TYPE }));
	handler->createIndex(databaseName, projectName + "." + REPO_COLLECTION_SCENE, Ascending({ REPO_NODE_LABEL_SHARED_ID }));
}

RepoSceneBuilder::AsyncImpl::AsyncImpl(RepoSceneBuilder* builder):
	builder(builder),
	block(0)
{
	consumer = std::thread(&RepoSceneBuilder::AsyncImpl::consumerFunction, this);
	threshold = DEFAULT_THRESHOLD;
}

RepoSceneBuilder::AsyncImpl::~AsyncImpl()
{
	push({ Consumables(Close()), 0 });
	consumer.join();
	if (consumerException) {
		std::rethrow_exception(consumerException);
	}
}

void RepoSceneBuilder::AsyncImpl::push(repo::core::handler::database::query::AddParent* u)
{
	push({ Consumables(u), 100 }); // Update operations have a fixed approximate cost
}

void RepoSceneBuilder::AsyncImpl::push(repo::core::model::RepoNode* node)
{
	push({ Consumables(node), node->getSize() });
}

void RepoSceneBuilder::AsyncImpl::push(Consumable consumable)
{
	// The block semaphore is used to introduce backpressure by prompting the main
	// thread (the caller) to suspend.

	// The threads are only loosely synchronised - we don't track memory to the byte,
	// and there is buffering in the bulk write context as well.
	// The use of semaphores means that the order of the acquire() and release()
	// calls should not matter (if the signal gets to the head of the consumer
	// before we move onto the next statement).

	// In any case though, to be sure there are no deadlocks we use try_acquire with
	// a timeout - in the worst case, this method will simply check 'size' again.

	if (consumable.size)
	{
		queueSize += consumable.size;
		while (queueSize > threshold) {
			queue.enqueue({ Consumables(Notify()), 0 });
			block.try_acquire_for(std::chrono::seconds(1));
			if (consumerException) {
				std::rethrow_exception(consumerException);
			}
		}
	}
	queue.enqueue(consumable);
}

RepoSceneBuilder::AsyncImpl::Consumer::Consumer(RepoSceneBuilder::AsyncImpl* impl):
	impl(impl)
{
	auto builder = impl->builder;
	auto handler = impl->builder->handler;
	collection = handler->getBulkWriteContext(builder->databaseName, builder->getSceneCollectionName());
}

bool RepoSceneBuilder::AsyncImpl::Consumer::operator() (repo::core::model::RepoNode* n) const
{
	// At the moment, profiling shows that this thread is often waiting more than
	// not, so we perform optimisations in it directly.
	// If this changes in the future, we will need to move the optimisation calls
	// somewhere else.

	auto meshNode = dynamic_cast<repo::core::model::MeshNode*>(n);
	if (meshNode) {
		meshNode->removeDuplicateVertices();
	}

	collection->insertDocument(*n);
	delete n;
	return true;
}

bool RepoSceneBuilder::AsyncImpl::Consumer::operator() (const repo::core::handler::database::query::AddParent* u) const
{
	collection->updateDocument(repo::core::handler::database::query::RepoUpdate(*u));
	delete u;
	return true;
}

bool RepoSceneBuilder::AsyncImpl::Consumer::operator() (const Close& n) const
{
	return false;
}

bool RepoSceneBuilder::AsyncImpl::Consumer::operator() (const Notify& n) const
{
	impl->block.release();
	return true;
}

void RepoSceneBuilder::AsyncImpl::consumerFunction()
{
	try {
		Consumer consumer(this);

		Consumable consumable;
		do {
			queue.wait_dequeue(consumable);
			queueSize -= consumable.size;
		} while (std::visit(consumer, consumable.object));
	}
	catch (...)
	{
		consumerException = std::current_exception();
	}
}

// It is required to tell the module which specialisations to instantiate
// for addNode.

#define DECLARE_ADDNODE_SPECIALISATION(T) template std::shared_ptr<T> RepoSceneBuilder::addNode<T>(const T& node);

DECLARE_ADDNODE_SPECIALISATION(MeshNode)
DECLARE_ADDNODE_SPECIALISATION(TransformationNode)
DECLARE_ADDNODE_SPECIALISATION(MaterialNode)
DECLARE_ADDNODE_SPECIALISATION(TextureNode)
DECLARE_ADDNODE_SPECIALISATION(MetadataNode)
