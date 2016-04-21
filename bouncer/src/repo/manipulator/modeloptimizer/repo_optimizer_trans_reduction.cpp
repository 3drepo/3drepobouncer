/**
*  Copyright (C) 2015 3D Repo Ltd
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

/**
* Transformation Reduction Optimizer
* Reduces the amount of transformations within the scene graph by
* merging transformation with its mesh (provided the mesh doesn't have
* multiple parent
*/

#include "repo_optimizer_trans_reduction.h"

using namespace repo::manipulator::modeloptimizer;

TransformationReductionOptimizer::TransformationReductionOptimizer() : AbstractOptimizer()
, gType(repo::core::model::RepoScene::GraphType::DEFAULT) //we only perform optimisation on default graphs
{
}

TransformationReductionOptimizer::~TransformationReductionOptimizer()
{
}

bool TransformationReductionOptimizer::apply(repo::core::model::RepoScene *scene)
{
	bool success = false;
	if (scene && scene->hasRoot(gType))
	{
		auto meshes = scene->getAllMeshes(gType);
		size_t count = 0;
		size_t total = meshes.size();
		size_t transNodes_pre = scene->getAllTransformations(gType).size();
		size_t step = total / 10;
		if (!step) step = total; //avoid modulo of 0;
		for (repo::core::model::RepoNode *node : meshes)
		{
			if (++count % step == 0)
			{
				repoInfo << "Optimizer : processed " << count << " of " << total << " meshes";
			}
			if (node && node->getTypeAsEnum() == repo::core::model::NodeType::MESH)
			{
				repo::core::model::MeshNode *mesh = dynamic_cast<repo::core::model::MeshNode*>(node);
				if (mesh)
					applyOptimOnMesh(scene, mesh);
				else
					repoError << "Failed to dynamically cast a mesh node!!!";
			}
		}

		repoInfo << "Mesh Optimisation complete. Number of transformations has been reduced from "
			<< transNodes_pre << " to " << scene->getAllTransformations(gType).size();

		transNodes_pre = scene->getAllTransformations(gType).size();
		auto cameras = scene->getAllCameras(gType);
		count = 0;
		total = cameras.size();
		for (repo::core::model::RepoNode *node : cameras)
		{
			++count;
			if (count % 100 == 0)
			{
				repoInfo << "Optimizer : processed " << count << " of " << total << " cameras";
			}
			if (node && node->getTypeAsEnum() == repo::core::model::NodeType::CAMERA)
			{
				repo::core::model::CameraNode *cam = dynamic_cast<repo::core::model::CameraNode*>(node);
				if (cam)
					applyOptimOnCamera(scene, cam);
				else
					repoError << "Failed to dynamically cast a camera node!!!";
			}
		}
		repoInfo << "Camera Optimisation complete. Number of transformations has been reduced from "
			<< transNodes_pre << " to " << scene->getAllTransformations(gType).size();
	}
	else
	{
		repoError << "Trying to apply optimisation on an empty scene!";
	}
	return success;
}

void TransformationReductionOptimizer::applyOptimOnMesh(
	repo::core::model::RepoScene *scene,
	repo::core::model::MeshNode  *mesh)
{
	/*
	* Assimp importer generates an extra transformation as a parent for a mesh
	* and metadata is tagged on the transformation rather than the mesh
	* even though the information is for the mesh
	* this function allows the mesh to consume the transformation and take its name.
	*/
	std::vector<repo::core::model::RepoNode*> transParents =
		scene->getParentNodesFiltered(gType,
		mesh, repo::core::model::NodeType::TRANSFORMATION);

	if (transParents.size() == 1)
	{
		repo::core::model::TransformationNode *trans =
			dynamic_cast<repo::core::model::TransformationNode*>(transParents[0]);
		if (trans)
		{
			repoUUID parentUniqueID = trans->getUniqueID();
			repoUUID parentSharedID = trans->getSharedID();
			repoUUID rootUniqueID = scene->getRoot(gType)->getUniqueID();
			bool isRoot = parentUniqueID == rootUniqueID;
			bool isIdentity = trans->isIdentity();
			if (!isRoot)
			{
				std::vector<repo::core::model::RepoNode*> children = scene->getChildrenAsNodes(gType, parentSharedID);
				bool singleMeshChild = scene->filterNodesByType(
					children, repo::core::model::NodeType::MESH).size() == 1;

				bool noTransSiblings = (bool)!scene->filterNodesByType(
					children, repo::core::model::NodeType::TRANSFORMATION).size() == 1;

				std::vector<repo::core::model::RepoNode*> granTransParents =
					scene->getParentNodesFiltered(gType,
					trans, repo::core::model::NodeType::TRANSFORMATION);

				if (singleMeshChild && noTransSiblings && granTransParents.size() == 1)
				{
					repo::core::model::TransformationNode *granTrans =
						dynamic_cast<repo::core::model::TransformationNode*>(granTransParents[0]);

					if (granTrans)
					{
						repoUUID granSharedID = granTrans->getSharedID();
						repoUUID meshSharedID = mesh->getSharedID();
						//Disconnect grandparent from parent
						scene->abandonChild(gType,
							granSharedID, trans, true, false);
						for (repo::core::model::RepoNode *node : children)
						{
							//Put all children of trans node to granTrans, unless it's a metadata node
							if (node)
							{
								scene->abandonChild(gType,
									parentSharedID, node, false, true);
								if (!isIdentity && node->positionDependant()){
									//Parent is not the identity matrix, we need to reapply the transformation if
									//the node is position dependant
									node->swap(node->cloneAndApplyTransformation(trans->getTransMatrix(false)));
								}

								//metadata should be assigned under the mesh
								scene->addInheritance(gType,
									node->getTypeAsEnum() == repo::core::model::NodeType::METADATA ? (repo::core::model::RepoNode*)mesh
									: (repo::core::model::RepoNode*) granTrans,
									node,
									false
									);
							}
						}

						//change mesh name
						repo::core::model::MeshNode newMesh = mesh->cloneAndChangeName(trans->getName(), false);

						scene->modifyNode(gType, mesh, &newMesh);

						//remove parent from the scene.
						scene->removeNode(gType, parentSharedID);
					}
					else
					{
						repoError << "Failed to dynamically cast a transformation node!!!!";
					}
				} //(singleMeshChild && noTransSiblings && granTransParents.size() == 1)
			}//(trans->getUniqueID() != scene->getRoot()->getUniqueID() && trans->isIdentity())
		}
		else
		{
			repoError << "Failed to dynamically cast a transformation node!!!!";
		}
	}
}

void TransformationReductionOptimizer::applyOptimOnCamera(
	repo::core::model::RepoScene *scene,
	repo::core::model::CameraNode  *camera)
{
	/*
	* Assimp importer generates an extra transformation as a parent for the camera
	* The camera and the transformation shares the same name.
	* This function will remove this transformation provided the camera name matches
	* the transformation's
	*/
	std::vector<repo::core::model::RepoNode*> transParents =
		scene->getParentNodesFiltered(gType,
		camera, repo::core::model::NodeType::TRANSFORMATION);

	if (transParents.size() == 1)
	{
		repo::core::model::TransformationNode *trans =
			dynamic_cast<repo::core::model::TransformationNode*>(transParents[0]);
		if (trans)
		{
			repoUUID parentUniqueID = trans->getUniqueID();
			repoUUID parentSharedID = trans->getSharedID();
			repoUUID rootUniqueID = scene->getRoot(gType)->getUniqueID();
			bool isRoot = parentUniqueID == rootUniqueID;
			bool isIdentity = trans->isIdentity();
			if (!isRoot)
			{
				std::vector<repo::core::model::RepoNode*> children = scene->getChildrenAsNodes(gType, parentSharedID);
				bool sameName = camera->getName() == trans->getName();

				bool noMeshSiblings = scene->filterNodesByType(
					children, repo::core::model::NodeType::MESH).size() == 0;

				bool noTransSiblings = (bool)!scene->filterNodesByType(
					children, repo::core::model::NodeType::TRANSFORMATION).size() == 1;

				std::vector<repo::core::model::RepoNode*> granTransParents =
					scene->getParentNodesFiltered(gType,
					trans, repo::core::model::NodeType::TRANSFORMATION);

				if (sameName && noMeshSiblings && noTransSiblings && granTransParents.size() == 1)
				{
					repo::core::model::TransformationNode *granTrans =
						dynamic_cast<repo::core::model::TransformationNode*>(granTransParents[0]);

					if (granTrans)
					{
						repoUUID granSharedID = granTrans->getSharedID();
						repoUUID camSharedID = camera->getSharedID();
						//Disconnect grandparent from parent
						scene->abandonChild(gType,
							granSharedID, trans, true, false);
						for (repo::core::model::RepoNode *node : children)
						{
							//Put all children of trans node to granTrans
							if (node)
							{
								scene->abandonChild(gType,
									parentSharedID, node, false, true);
								if (!isIdentity && node->positionDependant()){
									//Parent is not the identity matrix, we need to reapply the transformation if
									//the node is position dependant
									node->swap(node->cloneAndApplyTransformation(trans->getTransMatrix(false)));
								}

								//metadata should be assigned under the mesh
								scene->addInheritance(gType,
									granTrans, node, false);
							}
						}

						//change mesh name
						repo::core::model::MeshNode *newCam =
							new repo::core::model::MeshNode(camera->cloneAndChangeName(trans->getName(), false));

						scene->modifyNode(gType, camera, newCam);

						//remove parent from the scene.
						scene->removeNode(gType, parentSharedID);
						delete newCam;
					}
					else
					{
						repoError << "Failed to dynamically cast a transformation node!!!!";
					}
				} //(singleMeshChild && noTransSiblings && granTransParents.size() == 1)
			}//(trans->getUniqueID() != scene->getRoot()->getUniqueID() && trans->isIdentity())
		}
		else
		{
			repoError << "Failed to dynamically cast a transformation node!!!!";
		}
	}
}