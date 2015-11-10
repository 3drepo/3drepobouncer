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
{
}


TransformationReductionOptimizer::~TransformationReductionOptimizer() 
{
}

bool TransformationReductionOptimizer::apply(repo::core::model::RepoScene *scene)
{
	bool success = false;
	if (scene && scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		for (repo::core::model::RepoNode *node : scene->getAllMeshes())
		{
			if (node && node->getTypeAsEnum() == repo::core::model::NodeType::MESH)
			{
				repo::core::model::MeshNode *mesh = dynamic_cast<repo::core::model::MeshNode*>(node);
				if (mesh)
					applyOptimOnMesh(scene, mesh);
				else
					repoError << "Failed to dynamically cast a mesh node!!!";
			}
		}
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
	std::vector<repo::core::model::RepoNode*> transParents =
		scene->getParentNodesFiltered(repo::core::model::RepoScene::GraphType::DEFAULT,
		mesh, repo::core::model::NodeType::TRANSFORMATION);

	
	if (transParents.size() == 1)
	{
		repo::core::model::TransformationNode *trans = dynamic_cast<repo::core::model::TransformationNode*>(transParents[0]);
		if (trans)
		{
			repoUUID transUniqueID = trans->getUniqueID();
			repoUUID rootUniqueID = scene->getRoot(repo::core::model::RepoScene::GraphType::DEFAULT)->getUniqueID();
			bool isRoot = transUniqueID == rootUniqueID;
			bool isIdentity = trans->isIdentity();
			if (!isRoot)
			{
				bool singleMeshChild = scene->getChildrenNodesFiltered(repo::core::model::RepoScene::GraphType::DEFAULT,
					trans->getSharedID(), repo::core::model::NodeType::MESH).size() == 1;
				
				bool noTransSiblings = (bool) !scene->getChildrenNodesFiltered(repo::core::model::RepoScene::GraphType::DEFAULT,
					trans->getSharedID(), repo::core::model::NodeType::TRANSFORMATION).size();
				

				std::vector<repo::core::model::RepoNode*> granTransParents =
					scene->getParentNodesFiltered(repo::core::model::RepoScene::GraphType::DEFAULT,
					trans, repo::core::model::NodeType::TRANSFORMATION);


				if (singleMeshChild && noTransSiblings && granTransParents.size() == 1)
				{
					repo::core::model::TransformationNode *granTrans = 
						dynamic_cast<repo::core::model::TransformationNode*>(granTransParents[0]);

					if (granTrans)
					{
						repoUUID granSharedID = granTrans->getSharedID();
						repoUUID parentSharedID = trans->getSharedID();
						repoUUID meshSharedID = mesh->getSharedID();
						repoUUID mesholdID = mesh->getUniqueID();
						//Disconnect grandparent from parent
						scene->abandonChild(repo::core::model::RepoScene::GraphType::DEFAULT,
							granSharedID, parentSharedID, false);

						for (repo::core::model::RepoNode *node : scene->getChildrenAsNodes(trans->getSharedID()))
						{
							//Put all children of trans node to granTrans, unless it's a metadata node
							if (node)
							{

								scene->abandonChild(repo::core::model::RepoScene::GraphType::DEFAULT,
									parentSharedID, node->getSharedID(), true);
								if (!isIdentity && node->positionDependant()){
									//Parent is not the identity matrix, we need to reapply the transformation if 
									//the node is position dependant
									node->swap(node->cloneAndApplyTransformation(trans->getTransMatrix(false)));

									
								}

								//metadata should be assigned under the mesh
								scene->addInheritance(repo::core::model::RepoScene::GraphType::DEFAULT,
									node->getTypeAsEnum() == repo::core::model::NodeType::METADATA ? mesh->getUniqueID()
										: granTrans->getUniqueID(),
									node->getUniqueID(),
									false
									);
							}
						}

						//change mesh name
						repo::core::model::MeshNode *newMesh = 
							new repo::core::model::MeshNode(mesh->cloneAndChangeName(trans->getName(), false));
	
						scene->modifyNode(repo::core::model::RepoScene::GraphType::DEFAULT, meshSharedID, newMesh);

						//remove parent from the scene.
						scene->removeNode(repo::core::model::RepoScene::GraphType::DEFAULT, parentSharedID);
						delete newMesh;
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