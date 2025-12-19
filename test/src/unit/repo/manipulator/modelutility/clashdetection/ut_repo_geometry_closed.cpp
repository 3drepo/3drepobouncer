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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest-matchers.h>

#include <repo/manipulator/modelutility/clashdetection/geometry_tests_closed.h>
#include <repo/manipulator/modelutility/clashdetection/predicates.h>
#include <repo/manipulator/modelutility/clashdetection/bvh_operators.h>

#include "../../../../repo_test_utils.h"
#include "../../../../repo_test_clash_utils.h"
#include "../../../../repo_test_mesh_utils.h"
#include "../../../../repo_test_scene_utils.h"
#include "../../../../repo_test_matchers.h"
#include "../../../../repo_test_mock_clash_scene.h"
#include "../../../../repo_test_random_generator.h"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

/*
* The tests in this file evaluate the members of the geometry namespace. This
* is currently a dependency of the clash detection engine. 
* 
* The tests not only evaluate if the geometric tests are correct, but their
* robustness to rounding error, within the supported range of the clash engine.
* This is done probabilistically.
*/

using namespace testing;

// This convenience type is used to import Objs containing point clouds (as
// vertices) created by Blender. In these files such sets follow the object name
// and contain only vertices. (Which is not supported by the current version of
// Assimp used in Bouncer.)

// This type is only mean to work with the Objs in the clash tests folder
// and should not be used anywhere else.

// Some of the requirements of these Objs are:
// All objects must be named, all vertices must be listed after the object to
// which they belong, all faces must be triangles, faces must use positive
// indexing only, the file must begin with an object declaration.

struct SimpleObjImporter
{
	struct Object {
		std::vector<repo::lib::repo_face_t> faces;

		// Note that the faces index directly into the shared array, these ranges are
		// used when the object has no face to define the points within it.

		size_t vertexStart;
		size_t vertexEnd;
	};

	std::vector<repo::lib::RepoVector3D64> vertices;
	std::unordered_map<std::string, Object> objects;
	Object* currentObject = nullptr;

	void importObj(const std::string& filename) {
		std::ifstream file(filename);
		std::string line;
		while (std::getline(file, line)) {
			std::istringstream iss(line);
			std::string type;
			iss >> type;
			if (type == "o") {
				if (currentObject) {
					currentObject->vertexEnd = vertices.size();
				}
				std::string objectName;
				iss >> objectName;
				currentObject = &objects[objectName];
				currentObject->vertexStart = vertices.size();
			}
			else if (type == "v") {
				repo::lib::RepoVector3D64 v;
				iss >> v.x >> v.y >> v.z;
				vertices.push_back(v);
			}
			else if (type == "f") {
				repo::lib::repo_face_t f;
				iss >> f.operator[](0) >> f.operator[](1) >> f.operator[](2);
				f.operator[](0)--; f.operator[](1)--; f.operator[](2)--;
				currentObject->faces.push_back(f);
			}
		}
		if (currentObject) {
			currentObject->vertexEnd = vertices.size();
		}
	}
};

struct Problem
{
	std::vector<repo::lib::RepoTriangle> mesh;
	std::vector<repo::lib::RepoVector3D64> inside;
	std::vector<repo::lib::RepoVector3D64> outside;
};

std::unordered_map<std::string, Problem> importProblemsFromObj(std::string filename)
{
	SimpleObjImporter importer;
	importer.importObj(filename);

	std::unordered_map<std::string, Problem> problems;

	for (auto& [name, object] : importer.objects)
	{
		std::istringstream iss(name);
		std::vector<std::string> tokens;
		std::string token;
		while (std::getline(iss, token, '_')) {
			tokens.push_back(token);
		}
		auto& p = problems[tokens[0]];

		if (object.faces.size() > 0) {
			for (auto& face : object.faces) {
				p.mesh.push_back(
					repo::lib::RepoTriangle(
						importer.vertices[face[0]],
						importer.vertices[face[1]],
						importer.vertices[face[2]]
					)
				);
			}
		}
		else {
			for (size_t i = object.vertexStart; i < object.vertexEnd; i++) {
				auto v = importer.vertices[i];
				if (name.find("Inside") != std::string::npos) {
					p.inside.push_back(v);
				}
				else if (name.find("Outside") != std::string::npos) {
					p.outside.push_back(v);
				}
			}
		}
	}

	return problems;
}

TEST(Geometry, ClosedMeshContains)
{
	auto problems = importProblemsFromObj(
		getDataPath("clash/contains_1.obj")
	);

	for(auto& [name, p] : problems)
	{
		repo::lib::RepoBounds inside(p.inside.data(), p.inside.size());
		repo::lib::RepoBounds outside(p.outside.data(), p.outside.size());

		struct _MeshView : public geometry::MeshView
		{
			bvh::Bvh<double> bvh;
			const std::vector<repo::lib::RepoTriangle>& triangles;

			_MeshView(const std::vector<repo::lib::RepoTriangle>& triangles)
				: triangles(triangles)
			{
				bvh::builders::build(bvh, triangles);
			}
		
			repo::lib::RepoTriangle getTriangle(size_t primitive) const override {
				return triangles[primitive];
			}

			const bvh::Bvh<double>& getBvh() const override {
				return bvh;
			}
		};

		_MeshView  mesh(p.mesh);

		EXPECT_THAT(geometry::contains(p.inside, inside, mesh), IsTrue());

		// Note passing inside as the bounds here is deliberate, as it forces
		// the actual pointwise tests to be performed.

		EXPECT_THAT(geometry::contains(p.outside, inside, mesh), IsFalse());
	}
}