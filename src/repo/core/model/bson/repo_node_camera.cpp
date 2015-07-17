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
*  Camera node
*/

#include "repo_node_camera.h"

using namespace repo::core::model::bson;

CameraNode::CameraNode() :
RepoNode()
{
}

CameraNode::CameraNode(RepoBSON bson) :
RepoNode(bson)
{

}

CameraNode::~CameraNode()
{
}

CameraNode* CameraNode::createCameraNode(
	const float         &aspectRatio,
	const float         &farClippingPlane,
	const float         &nearClippingPlane,
	const float         &fieldOfView,
	const repo_vector_t &lookAt,
	const repo_vector_t &position,
	const repo_vector_t &up,
	const int           &apiLevel,
	const std::string   &name)
{
	RepoBSONBuilder builder;

	//--------------------------------------------------------------------------
	// Compulsory fields such as _id, type, api as well as path
	// and optional name
	appendDefaults(builder, REPO_NODE_TYPE_CAMERA, apiLevel, generateUUID(), name);

	//--------------------------------------------------------------------------
	// Aspect ratio
	builder << REPO_NODE_LABEL_ASPECT_RATIO << aspectRatio;

	//--------------------------------------------------------------------------
	// Far clipping plane
	builder << REPO_NODE_LABEL_FAR << farClippingPlane;

	//--------------------------------------------------------------------------
	// Near clipping plane
	builder << REPO_NODE_LABEL_NEAR << nearClippingPlane;

	//--------------------------------------------------------------------------
	// Field of view
	builder << REPO_NODE_LABEL_FOV << fieldOfView;

	//--------------------------------------------------------------------------
	// Look at vector
	builder.appendVector(REPO_NODE_LABEL_LOOK_AT, lookAt);

	//--------------------------------------------------------------------------
	// Position vector 
	builder.appendVector(REPO_NODE_LABEL_POSITION, position);

	//--------------------------------------------------------------------------
	// Up vector
	builder.appendVector(REPO_NODE_LABEL_UP, up);

	return new CameraNode(builder.obj());
}
