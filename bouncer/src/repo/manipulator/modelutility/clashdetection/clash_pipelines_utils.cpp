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

#include "clash_pipelines_utils.h"

#include "hopscotch-map/bhopscotch_map.h"

using namespace repo::manipulator::modelutility::clash;

void PipelineUtils::loadGeometry(
	DatabasePtr handler,
	Graph::Node& node,
	std::vector<repo::lib::RepoVector3D64>& vertices,
	std::vector<repo::lib::repo_face_t>& faces
)
{
	handler->loadBinaryBuffers(
		node.container->teamspace,
		node.container->container + "." + REPO_COLLECTION_SCENE,
		node.mesh
	);

	auto mesh = repo::core::model::MeshNode(node.mesh);

	faces = mesh.getFaces();

	vertices.reserve(mesh.getNumVertices());
	for(auto& v : mesh.getVertices()) {
		// Put the meshes in project coordinates. We create new arrays below
		// as we want to keep the vertices in double precision from now on.
		vertices.push_back(node.matrix * repo::lib::RepoVector3D64(v));
	}

	node.mesh.unloadBinaryBuffers();
}