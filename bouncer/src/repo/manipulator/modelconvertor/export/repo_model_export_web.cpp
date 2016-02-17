/**
*  Copyright (C) 2016 3D Repo Ltd
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
* Allows Export functionality from 3D Repo World to SRC
*/


#include "repo_model_export_web.h"
#include "../../../lib/repo_log.h"
#include "../../../core/model/bson/repo_bson_factory.h"

using namespace repo::manipulator::modelconvertor;

WebModelExport::WebModelExport(
	const repo::core::model::RepoScene *scene
	) : AbstractModelExport(scene)
{
	if (convertSuccess = scene)
	{

		if (scene->hasRoot(repo::core::model::RepoScene::GraphType::OPTIMIZED))
		{
			gType = repo::core::model::RepoScene::GraphType::OPTIMIZED;
		}
		else if (scene->hasRoot(repo::core::model::RepoScene::GraphType::DEFAULT))
		{
			gType = repo::core::model::RepoScene::GraphType::DEFAULT;
		}
		else
		{
			repoError << "Failed to export to scene : Failed to find root node within the scene!";
			convertSuccess = false;
		}
	}
}

WebModelExport::~WebModelExport()
{
}


std::string WebModelExport::getSupportedFormats()
{
	return ".src, .gltf";
}
