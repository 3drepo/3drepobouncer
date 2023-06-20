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
* Mesh Simplification Optimizer
* Performs decimation on compatible meshes to reduce vertex count at the cost
* of quality.
*/

#include "repo_optimizer_mesh_simplification.h"
#include "../../core/model/bson/repo_node_transformation.h"
#include "../../lib/repo_log.h"

#include <vector>
#include <chrono>

#include <ifcUtils/repo_ifc_utils_constants.h>

// One of the bouncer dependencies defines these in a header, inteferring with 
// proper declarations below
#undef max
#undef min

#include "pmp/SurfaceMesh.h"
#include "pmp/algorithms/Merge.h"
#include "pmp/algorithms/Manifold.h"
#include "pmp/algorithms/Normals.h"
#include "pmp/algorithms/Decimation.h"
#include "pmp/algorithms/Triangulation.h"
#include "pmp/BoundingBox.h"
#include "pmp/utilities.h";
#include "pmp/io/write_pmp.h"

#define CHRONO_DURATION(start) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count()

using namespace repo::manipulator::modeloptimizer;

class MeshSimplificationOptimizer::Mesh : public pmp::SurfaceMesh
{
};

MeshSimplificationOptimizer::MeshSimplificationOptimizer(
	const double quality,
	const int minVertexCount) :
	AbstractOptimizer(), 
	gType(repo::core::model::RepoScene::GraphType::DEFAULT), //we only perform optimisation on default graphs
	quality(quality),
	minVertexCount(minVertexCount)
{
}

MeshSimplificationOptimizer::~MeshSimplificationOptimizer()
{
}

static repo::core::model::MeshNode currentMeshNode;

#ifdef _WIN32
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal.
	case CTRL_C_EVENT:
		printf("Ctrl-C event\n\n");
		try {
			MeshSimplificationOptimizer::Mesh original;
			MeshSimplificationOptimizer::convertMeshNode(&currentMeshNode, original);
			std::string path = "D:\\3drepo\\ISSUE_599\\exception\\" + currentMeshNode.getUniqueID().toString();
			pmp::IOFlags flags;
			flags.use_vertex_normals = true;
			pmp::write_pmp(original, path, flags);
		}
		catch (std::exception e) {
			// Failed twice, no way we can get the mesh...
		}
		return TRUE;

	default:
		return FALSE;
	}
}
#endif


bool MeshSimplificationOptimizer::apply(repo::core::model::RepoScene *scene)
{
#ifdef _WIN32
	if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
	{
		repoInfo << "Control Handler Registered";
	}
#endif

	auto start = std::chrono::high_resolution_clock::now();
	size_t startingVertexCount = 0;
	size_t endingVertexCount = 0;
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
					startingVertexCount += mesh->getNumVertices();

					if (shouldOptimizeMeshNode(scene, mesh)) {
						mesh->swap(optimizeMeshNode(mesh));
					}

					endingVertexCount += mesh->getNumVertices();
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
	repoInfo << "MeshSimplificationOptimizer completed in " << CHRONO_DURATION(start) << " milliseconds.";
	repoInfo << "MeshSimplificationOptimizer reduced model from " << startingVertexCount << " to " << endingVertexCount << " vertices.";

	return success;
}

double MeshSimplificationOptimizer::getVolume(repo::core::model::MeshNode* node)
{
	auto bounds = node->getBoundingBox();
	auto x = bounds[1].x - bounds[0].x;
	auto y = bounds[1].y - bounds[0].y;
	auto z = bounds[1].z - bounds[0].z;
	return x * y * z;
}

double MeshSimplificationOptimizer::getQuality(repo::core::model::MeshNode* node)
{
	return (double)node->getNumVertices() / getVolume(node);
}

bool MeshSimplificationOptimizer::shouldOptimizeMeshNode(repo::core::model::RepoScene* scene, repo::core::model::MeshNode* node)
{
	if (!canOptimizeMeshNode(scene, node))
	{
		return false;
	}
	
	if (getQuality(node) <= quality)
	{
		return false;
	}

	return true;
}

bool MeshSimplificationOptimizer::canOptimizeMeshNode(repo::core::model::RepoScene* scene, repo::core::model::MeshNode* node)
{
	// Check if the mesh can be simplified with the current implementation.

	// Some meshes may have UVs by default, but no textures assigned, in which 
	// case the UVs should be dropped.

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

	if(node->getNumVertices() < minVertexCount)
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
	// This method assumes a property arrays are contiguous, so make sure this is
	// the case.
	mesh.garbage_collection();

	std::vector<uint32_t> level1faces; // (Api Level 1 Faces prepend each face with the number of indices, in this case always 3.)
	for (auto face : mesh.faces())
	{
		level1faces.push_back(3);
		for (auto vertex : mesh.vertices(face)) 
		{
			level1faces.push_back(vertex.idx());
		}
	}

	std::vector<repo::lib::RepoVector3D> vertices;
	vertices.resize(mesh.n_vertices());
	memcpy(vertices.data(), mesh.get_vertex_property<pmp::Point>("v:point").data(), mesh.n_vertices() * sizeof(repo::lib::RepoVector3D));

	std::vector<repo::lib::RepoVector3D> normals;
	normals.resize(mesh.n_vertices());
	memcpy(normals.data(), mesh.get_vertex_property<pmp::Normal>("v:normal").data(), mesh.n_vertices() * sizeof(repo::lib::RepoVector3D));

	return node->cloneAndUpdateGeometry(
		vertices,
		normals,
		level1faces
	);
}

repo::core::model::MeshNode MeshSimplificationOptimizer::optimizeMeshNode(repo::core::model::MeshNode* meshNode)
{
	try {
		// do the simplification here...
		repoInfo << "Simplifying mesh " << meshNode->getName();

		currentMeshNode = *meshNode;//todo: debug

		MeshSimplificationOptimizer::Mesh mesh;
		convertMeshNode(meshNode, mesh);

		auto maxVertices = std::max(getVolume(meshNode) * quality, (double)minVertexCount);

		// The first step is to collapse coincident vertices; most importers will 
		// generate multiple vertices when encountering per-vertex normals, which
		// prevent the identification of common edges to collapse.

		pmp::Merge merge(mesh);
		merge.merge();

		pmp::Decimation decimation(mesh);
		decimation.initialize(0.0, 0.0, 0.0, 30.0, 0.0);
		decimation.decimate(maxVertices);

		// Finally make sure the mesh is still a pure triangle mesh since that
		// is the highest order primitive we handle.

		bool isTriangular = true;
		for (auto f : mesh.faces())
		{
			if (mesh.valence(f) != 3) {
				isTriangular = false;
			}
		}

		if (!isTriangular) {

			pmp::Manifold(mesh).fix_manifold();
			pmp::Triangulation(mesh).triangulate();
		}

		repoInfo << "Reduced mesh from " << meshNode->getNumVertices() << " to " << mesh.n_vertices() << " vertices (target " << maxVertices << " vertices)";

		return updateMeshNode(meshNode, mesh);
	}
	catch (std::exception e)
	{
		repoInfo << "Exception " << e.what() << " simplifying mesh " << meshNode->getUniqueID().toString();

		// uncomment this to have the bouncer output a pmp of any failed meshes so the algorithm can be run in isolation
#ifdef _WIN32
		try {
			MeshSimplificationOptimizer::Mesh original;
			convertMeshNode(meshNode, original);
			std::string path = "D:\\3drepo\\ISSUE_599\\exception\\" + meshNode->getUniqueID().toString();
			pmp::IOFlags flags;
			flags.use_vertex_normals = true;
			pmp::write_pmp(original, path, flags);
		}
		catch (std::exception e) {
			// Failed twice, no way we can get the mesh...
		}
#endif		
		return *meshNode;
	}
}


