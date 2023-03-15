/**
*  Copyright (C) 2023 3D Repo Ltd
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

#include "repo_optimizer_mesh_simplification.h"
#include "../../core/model/bson/repo_node_transformation.h"
#include "../../lib/repo_log.h"

#include <vector>

#include <ifcUtils/repo_ifc_utils_constants.h>

// One of the bouncer dependencies defines these in a header, inteferring with 
// proper declarations below
#undef max
#undef min

#include "pmp/SurfaceMesh.h"
#include "pmp/algorithms/Normals.h"

using namespace repo::manipulator::modeloptimizer;

class MeshSimplificationOptimizer::Mesh : public pmp::SurfaceMesh
{
};

MeshSimplificationOptimizer::MeshSimplificationOptimizer(
	const int quality) :
	AbstractOptimizer()
	, gType(repo::core::model::RepoScene::GraphType::DEFAULT) //we only perform optimisation on default graphs
	, quality(quality)
{
}

MeshSimplificationOptimizer::~MeshSimplificationOptimizer()
{
}

bool MeshSimplificationOptimizer::apply(repo::core::model::RepoScene *scene)
{
	bool success = false;
	if (scene && scene->hasRoot(gType))
	{
		auto meshes = scene->getAllMeshes(gType);
		
		size_t total = meshes.size();
		size_t step = total / 10;
		if (!step) step = total; //avoid modulo of 0;

		for (repo::core::model::RepoNode *node : meshes)
		{
			if (node && node->getTypeAsEnum() == repo::core::model::NodeType::MESH)
			{
				repo::core::model::MeshNode *mesh = dynamic_cast<repo::core::model::MeshNode*>(node);
				if (mesh)
				{
					if (shouldOptimizeMeshNode(scene, mesh)) {
						// todo:: check if we want to do this, or use the mesh->swap() member
						scene->modifyNode(
							gType,
							mesh,
							&optimizeMeshNode(mesh),
							true
						);
					}
				}
				else
				{
					repoError << "Failed to dynamically cast a mesh node";
				}
			}
		}
	}
	else
	{
		repoError << "Trying to apply optimisation on an empty scene!";
	}
	return success;
}

bool MeshSimplificationOptimizer::shouldOptimizeMeshNode(repo::core::model::RepoScene* scene, repo::core::model::MeshNode* node)
{
	// Check if the mesh can be simplified with the current
	// implementation.

	// Some meshes may have UVs by default, but no textures assigned, in which case the UVs should be ignored
	// Todo:: when we support UVs, we need to distinguish whether we should keep them or not
	if (node->getNumUVChannels() > 0 && scene->getTextureIDForMesh(gType, node->getSharedID()).length() > 0)
	{
		return false;
	}
	if (node->getPrimitive() != repo::core::model::MeshNode::Primitive::TRIANGLES)
	{
		return false;
	}
	if (node->getColors().size() > 0)
	{
		return false;
	}

	return true;
}

void MeshSimplificationOptimizer::convertMeshNode(repo::core::model::MeshNode* node, Mesh& mesh)
{
	auto vertices = node->getVertices();
	auto faces = node->getFaces();

	std::vector<pmp::Vertex> vertexPointers;

	for (size_t i = 0; i < vertices.size(); i++)
	{
		vertexPointers.push_back(
			mesh.add_vertex(pmp::Point(
				vertices[i].x,
				vertices[i].y,
				vertices[i].z
			)));
	}

	for (auto face : faces)
	{
		mesh.add_triangle(
			vertexPointers[face[0]],
			vertexPointers[face[1]],
			vertexPointers[face[2]]
		);
	}

	// The simplification requires normals; add them to the mesh if we have them,
	// or compute them if we don't.

	auto normals = node->getNormals();
	if (normals.size() == vertices.size())
	{
		auto mesh_normals = mesh.add_vertex_property<pmp::Point>("v:normal");
		
		for (size_t i = 0; i < vertices.size(); i++)
		{
			mesh_normals[vertexPointers[i]] = pmp::Point(
				normals[i].x,
				normals[i].y,
				normals[i].z
			);
		}
	}
	else
	{
		pmp::Normals::compute_vertex_normals(mesh);
	}
}

repo::core::model::MeshNode MeshSimplificationOptimizer::updateMeshNode(repo::core::model::MeshNode* node, Mesh& mesh)
{
	std::vector<uint32_t> simplexIndices;
	for (auto face : mesh.faces())
	{
		simplexIndices.push_back(3);
		for (auto vertex : mesh.vertices(face)) 
		{
			simplexIndices.push_back(vertex.idx());
		}
	}

	std::vector<repo::lib::RepoVector3D> vertices;
	vertices.resize(mesh.n_vertices());
	memcpy(vertices.data(), mesh.get_vertex_property<pmp::Point>("v:point").data(), mesh.n_vertices() * sizeof(repo::lib::RepoVector3D));

	std::vector<repo::lib::RepoVector3D> normals;
	normals.resize(mesh.n_vertices());
	memcpy(normals.data(), mesh.get_vertex_property<pmp::Point>("v:normal").data(), mesh.n_vertices() * sizeof(repo::lib::RepoVector3D));

	return node->cloneAndUpdateGeometry(
		vertices,
		normals,
		simplexIndices
	);
}

repo::core::model::MeshNode MeshSimplificationOptimizer::optimizeMeshNode(repo::core::model::MeshNode* meshNode)
{
	MeshSimplificationOptimizer::Mesh mesh;
	convertMeshNode(meshNode, mesh);

	// do the simplification here...
	repoInfo << "Converting mesh... ";

	auto positions = mesh.get_vertex_property<pmp::Point>("v:point");
	for (auto v : mesh.vertices()) {
		positions[v] = pmp::Point(
			positions[v](0, 0) * 1.2f,
			positions[v](1, 0),
			positions[v](2, 0)
		);
	}	

	return updateMeshNode(meshNode, mesh);
}
