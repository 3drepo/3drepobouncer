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
* Transformation Node
*/

#pragma once
#include "repo_node.h"

namespace repo {
	namespace core {
		namespace model {

				//------------------------------------------------------------------------------
				//
				// Fields specific to transformation only
				//
				//------------------------------------------------------------------------------

				#define REPO_NODE_LABEL_MATRIX						"matrix"
				//------------------------------------------------------------------------------

				class REPO_API_EXPORT TransformationNode :public RepoNode
				{
					public:

						/**
						* Default constructor
						*/
						TransformationNode();

						/**
						* Construct a TransformationNode from a RepoBSON object
						* @param RepoBSON object 
						*/
						TransformationNode(RepoBSON bson);


						/**
						* Default deconstructor
						*/
						~TransformationNode();

						/**
						* Check if the transformation matrix is the identity matrix
						* This checks with a small epsilon to counter floating point inaccuracies
						* @param eps epsilon value for accepting inaccuracies (default 10e-5)
						* @return returns true if it is the identity matrix
						*/
						bool isIdentity(const float &eps = 10e-5) const;
						/**
						* Create an Identity matrix
						* @return returns a 4 by 4 identity matrix
						*/
						static std::vector<std::vector<float>> identityMat();

						/**
						* --------- Convenience functions -----------
						*/

						/**
						* Get the 4 by 4 transformation matrix
						* @param true if row major (row is the fast dimension)
						* @return returns the 4 by 4 matrix as a vector
						*/
						std::vector<float> getTransMatrix(const bool &rowMajor = true) const;


				};
		} //namespace model
	} //namespace core
} //namespace repo


