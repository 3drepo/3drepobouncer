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
* Camera Node
*/

#pragma once
#include "repo_node.h"

namespace repo {
	namespace core {
		namespace model {

				//------------------------------------------------------------------------------
				//
				// Fields specific to camera only
				//
				//------------------------------------------------------------------------------
				#define REPO_NODE_LABEL_ASPECT_RATIO		"aspect_ratio"
				#define REPO_NODE_LABEL_FAR					"far"
				#define REPO_NODE_LABEL_NEAR				"near"
				#define REPO_NODE_LABEL_FOV					"fov"
				#define REPO_NODE_LABEL_LOOK_AT				"look_at"
				#define REPO_NODE_LABEL_POSITION			"position"
				#define REPO_NODE_LABEL_UP					"up"
				#define REPO_NODE_UUID_SUFFIX_CAMERA		"04" //!< uuid suffix
				//------------------------------------------------------------------------------


				class REPO_API_EXPORT CameraNode :public RepoNode
				{
				public:

					/**
					* Default constructor
					*/
					CameraNode();

					/**
					* Construct a CameraNode from a RepoBSON object
					* @param RepoBSON object
					*/
					CameraNode(RepoBSON bson);


					/**
					* Default deconstructor
					*/
					~CameraNode();

					/**
					* Static builder for factory use to create a Camera Node
					* @param aspect ratio
					* @param Far clipping plane
					* @param Near clipping plane. Should not be 0 to avoid divis
					* @param Field of view.
					* @param LookAt vector relative to parent transformations.
					* @param Position relative to parent transformations
					* @param Up vector relative to parent transformations.
					* @param API level of the node (optional, default REPO_NODE_API_LEVEL_1)
					* @param name of the node (optional, default empty string)
					* @return returns a pointer Camera node
					*/
					static CameraNode createCameraNode(
						const float         &aspectRatio,
						const float         &farClippingPlane,
						const float         &nearClippingPlane,
						const float         &fieldOfView,
						const repo_vector_t &lookAt,
						const repo_vector_t &position,
						const repo_vector_t &up,
						const int           &apiLevel = REPO_NODE_API_LEVEL_1,
						const std::string   &name = std::string());


					/**
					* --------- Convenience functions -----------
					*/

					/**
					* Get aspect ratio
					* @return returns the 4 by 4 matrix as a vector
					*/
					float getAspectRatio() const
					{
						return hasField(REPO_NODE_LABEL_ASPECT_RATIO) ?
							(float) getField(REPO_NODE_LABEL_ASPECT_RATIO).numberDouble() :
							1.;
					}

					/**
					* Get horiztonal FOV
					* @return returns the 4 by 4 matrix as a vector
					*/
					float getHorizontalFOV() const
					{
						return hasField(REPO_NODE_LABEL_FOV) ?
							(float) getField(REPO_NODE_LABEL_FOV).numberDouble() :
							1.;
					}

					/**
					* Get the 4 by 4 camera matrix
					* @param true if row major (row is the fast dimension)
					* @return returns the 4 by 4 matrix as a vector
					*/
					std::vector<float> getCameraMatrix(const bool &rowMajor = true) const;

					/**
					* Return the value for far clipping plane
					* @return returns the value for far clipping plane
					*/
					float getFarClippingPlane() const
					{
						return hasField(REPO_NODE_LABEL_FAR) ?
							(float)getField(REPO_NODE_LABEL_FAR).numberDouble() :
							1.;
					}


					/**
					* Return the value for field of view
					* @return returns the value for fieldOfView
					*/
					float getFieldOfView() const
					{
						return hasField(REPO_NODE_LABEL_FOV) ?
							(float)getField(REPO_NODE_LABEL_FOV).numberDouble() :
							1.;
					}

					/**
					* Return the value for near clipping plane
					* @return returns the value for near clipping plane
					*/
					float getNearClippingPlane() const
					{
						return hasField(REPO_NODE_LABEL_NEAR) ?
							(float)getField(REPO_NODE_LABEL_NEAR).numberDouble() :
							1.;
					}

					/**
					* Get the Look At vector of the camera
					* @return returns a vector of the "Look At" 
					*/
					repo_vector_t getLookAt() const;

					/**
					* Get the position of the camera
					* @return returns a vector of the position
					*/
					repo_vector_t getPosition() const;

					/**
					* Get the up vector of the camera
					* @return returns a vector of up
					*/
					repo_vector_t getUp() const;

				};
		} //namespace model
	} //namespace core
} //namespace repo


