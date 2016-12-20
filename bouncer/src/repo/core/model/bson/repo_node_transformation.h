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
				* Get the type of node
				* @return returns the type as a string
				*/
				virtual std::string getType() const
				{
					return REPO_NODE_TYPE_TRANSFORMATION;
				}

				/**
				* Get the type of node as an enum
				* @return returns type as enum.
				*/
				virtual NodeType getTypeAsEnum() const
				{
					return NodeType::TRANSFORMATION;
				}

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

				/**
				* Check if the node is semantically equal to another
				* Different node should have a different interpretation of what
				* this means.
				* @param other node to compare with
				* @param returns true if equal, false otherwise
				*/
				virtual bool sEqual(const RepoNode &other) const;

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
					const repo::lib::RepoMatrix &matrix) const;

				/**
				* Create a new object with the same values, but the transformation being reset
				* identity matrix
				* @return returns a new transformation node with identity matrix
				*/
				virtual TransformationNode cloneAndResetMatrix() const;

				/**
				* --------- Convenience functions -----------
				*/

				/**
				* Get the 4 by 4 transformation matrix
				* @param true if row major (row is the fast dimension)
				* @return returns the 4 by 4 matrix as a vector
				*/
				repo::lib::RepoMatrix getTransMatrix(const bool &rowMajor) const;
			};
		} //namespace model
	} //namespace core
} //namespace repo
