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

#include "repo_clash_detection_engine.h"

#include <repo/lib/datastructure/repo_matrix.h>
#include <repo/lib/datastructure/repo_bounds.h>
#include <repo/lib/datastructure/repo_triangle.h>
#include <repo/core/handler/repo_database_handler_abstract.h>
#include <repo/core/handler/database/repo_query.h>
#include <repo/core/model/repo_model_global.h>
#include <repo/core/model/bson/repo_bson.h>
#include <repo/core/model/bson/repo_node.h>
#include <repo/core/model/bson/repo_node_mesh.h>
#include <repo/core/model/bson/repo_node_transformation.h>
#include <repo/core/model/bson/repo_node_streaming_mesh.h>

#include "clashdetection/clash_hard.h"
#include "clashdetection/clash_clearance.h"
#include "clashdetection/sparse_scene_graph.h"

#include <repo/manipulator/modeloptimizer/bvh/bvh.hpp>
#include <repo/manipulator/modeloptimizer/bvh/sweep_sah_builder.hpp>

using namespace repo::lib;
using namespace repo::manipulator::modelutility;
using namespace repo::manipulator::modelutility::clash;

struct NodeReference {
	// This can be unique or shared Id, depending on the stage.
	repo::lib::RepoUUID id;

	// The destination graph that this node should be read into.
	sparse::SceneGraph& sceneGraph;
};

using ContainerGroups = std::unordered_map<const repo::lib::Container*, std::vector<repo::lib::RepoUUID>>;
using DatabasePtr = std::shared_ptr<repo::core::handler::AbstractDatabaseHandler>;
using Bvh = bvh::Bvh<double>;


ClashDetectionReport ClashDetectionEngine::runClashDetection
	(const ClashDetectionConfig& config)
{

	clash::Pipeline* pipeline = nullptr;
	switch (config.type) {
	case ClashDetectionType::Clearance:
		pipeline = new clash::Clearance(handler, config);
	case ClashDetectionType::Hard:
		pipeline = new clash::Hard(handler, config);
	default:
		throw std::invalid_argument("Unknown clash detection type");
	}

	return pipeline->runPipeline();
}


ClashDetectionEngine::ClashDetectionEngine(std::shared_ptr<repo::core::handler::AbstractDatabaseHandler> handler)
	:handler(handler)
{
}