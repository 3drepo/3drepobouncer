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
					* Check if the node is position dependant.
					* i.e. if parent transformation is merged onto the node,
					* does the node requre to a transformation applied to it
					* e.g. meshes and cameras are position dependant, metadata isn't
					* Default behaviour is false. Position dependant child requires
					* override this function.
					* @return true if node is positionDependant.
					*/
					virtual bool positionDependant() { return true; }

					/*
					*	------------- Delusional modifiers --------------
					*   These are like "setters" but not. We are actually
					*   creating a new bson object with the changed field
					*/

					/**
					*  Create a new object with transformation applied to the node
					* default behaviour is do nothing. Children object
					* needs to override this function to perform their own specific behaviour.
					* @param matrix transformation matrix to apply.
					* @return returns a new object with transformation applied.
					*/
					virtual RepoNode cloneAndApplyTransformation(
						const std::vector<float> &matrix) const;

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

					/**
					* Check if the node is semantically equal to another
					* Different node should have a different interpretation of what
					* this means.
					* @param other node to compare with
					* @param returns true if equal, false otherwise
					*/
					virtual bool sEqual(const RepoNode &other) const;
				};
		} //namespace model
	} //namespace core
} //namespace repo


