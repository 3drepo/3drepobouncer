/**
*  Copyright (C) 2014 3D Repo Ltd
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
* A Repo project (a project consists of a scene graph and a revision graph)
*/


#include "repo_project.h"


using namespace repo::manipulator;

RepoProject::RepoProject(
	repo::core::handler::AbstractDatabaseHandler *dbHandler,
	std::string projectName,
	std::string database,
	std::string sceneExt,
	std::string revExt)
	: projectName (projectName),
	  databaseName(database),
	  dbHandler   (dbHandler),
	  sceneExt    (sceneExt),
	  revExt      (revExt),
	  revNode(0)
{
	//defaults to master branch
	branch = stringToUUID(REPO_HISTORY_MASTER_BRANCH);
	//instantiate scene and revision graph
	sceneGraph = new graph::SceneGraph();

}

RepoProject::~RepoProject()
{
	delete sceneGraph;
	delete revNode;
}

bool RepoProject::loadRevision(std::string &errMsg){
	bool success = true;
	repo::core::model::bson::RepoBSON bson;
	if (headRevision){

		bson = dbHandler->findOneBySharedID(databaseName, projectName + "." +
			revExt, branch, REPO_NODE_REVISION_LABEL_TIMESTAMP);
	}
	else{
		bson = dbHandler->findOneByUniqueID(databaseName, projectName + "." + revExt, revision);
	}

	if (bson.isEmpty()){
		errMsg = "Failed: cannot find revision document from " + databaseName + "." + projectName + "." + revExt;
		success = false;
	}
	else{
		revNode = new repo::core::model::bson::RevisionNode(bson);
	}

	return success;
}

void RepoProject::loadScene(){
	if (!revNode){
		//Load revision object first
	}


}
