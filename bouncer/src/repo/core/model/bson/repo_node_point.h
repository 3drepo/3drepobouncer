/**
*  Copyright (C) 2026 3D Repo Ltd
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

#pragma once
#include "repo_node.h"
#include "repo/repo_bouncer_global.h"
#include "repo/lib/datastructure/repo_structs.h"
#include "repo/lib/datastructure/repo_bounds.h"

namespace repo {
	namespace core {
		namespace model {

			//------------------------------------------------------------------------------
//
// Fields specific only to point clouds
//
//------------------------------------------------------------------------------
#define REPO_NODE_POINT_LABEL_BOUNDING_BOX		"bounding_box"		//!< bounding box
#define REPO_NODE_POINT_LABEL_TREE_POSITION		"tree_position"		//!< tree position
#define REPO_NODE_POINT_LABEL_POINTS			"points"			//!< point array
#define REPO_NODE_POINT_LABEL_POINTS_COUNT		"points_count"		//!< points size
#define REPO_NODE_POINT_LABEL_COLOURS			"colour_attribute"	//!< colour attribute
// #define REPO_NODE_POINT_LABEL_ATTRIBUTES		"attributes"		//!< attributes

			class REPO_API_EXPORT PointNode : public RepoNode
			{
			public:
				// Default constructor
				PointNode();

				/// <summary>
				/// Construct a PointNode from a RepoBSON object
				/// </summary>
				/// <param name="bson"></param>
				PointNode(RepoBSON bson);

				// Default deconstructor
				~PointNode();

			protected:
				virtual void deserialise(RepoBSON&);
				virtual void serialise(repo::core::model::RepoBSONBuilder&) const;

				repo::lib::RepoBounds boundingBox; // TODO FT: Double check whether we really want to store this or generate from tree position later.
				std::vector<uint8_t> treePosition;
				std::vector<repo::lib::RepoVector3D> points;
				std::vector<repo::lib::repo_color4d_t> colourAttributes;
				// std::vector<std::vector<float>> attributes;

			public:
				virtual std::string getType() const
				{
					return REPO_NODE_TYPE_POINT;
				}

				virtual NodeType getTypeAsEnum() const
				{
					return NodeType::POINT;
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
				virtual bool positionDependant()
				{
					return true;
				}

				/**
				* Check if the node is semantically equal to another
				* Different node should have a different interpretation of what
				* this means.
				* @param other node to compare with
				* @param returns true if equal, false otherwise
				*/
				virtual bool sEqual(const RepoNode& other) const;

				/**
				*  Create a new object with transformation applied to the node
				* default behaviour is do nothing. Children object
				* needs to override this function to perform their own specific behaviour.
				* @param matrix transformation matrix to apply.
				* @return returns a new object with transformation applied.
				*/
				// TODO FT: Revisit whether we really need this
				PointNode cloneAndApplyTransformation(
					const repo::lib::RepoMatrix& matrix) const;

				// TODO FT: Revisit whether we really need this
				void applyTransformation(
					const repo::lib::RepoMatrix& matrix);

				repo::lib::RepoBounds getBoundingBox() const
				{
					return boundingBox;
				}

				void setBoundingBox(const repo::lib::RepoBounds& bounds)
				{
					boundingBox = bounds;
				}

				// Retrieve a vector of points from the object
				const std::vector<repo::lib::RepoVector3D>& getPoints() const
				{
					return points;
				}

				void setPoints(const std::vector<repo::lib::RepoVector3D>& points)
				{
					this->points = std::vector<repo::lib::RepoVector3D>(points.begin(), points.end());
				}

				std::uint32_t getNumPoints() const
				{
					return points.size();
				}

				const std::vector<repo::lib::repo_color4d_t>& getColourAttributes() const
				{
					return colourAttributes;
				}

				void setColourAttributes(const std::vector<repo::lib::repo_color4d_t>& colourAttributes)
				{
					this->colourAttributes = std::vector<repo::lib::repo_color4d_t>(colourAttributes.begin(), colourAttributes.end());
				}

				const std::vector<uint8_t>& getTreePosition() const
				{
					return treePosition;
				}

				void setTreePosition(const std::vector<uint8_t>& treePosition)
				{
					this->treePosition = std::vector<uint8_t>(treePosition.begin(), treePosition.end());
				}

			};
		} //namespace model
	} //namespace core
} //namespace repo