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

#pragma once

#include "repo/manipulator/modelutility/repo_scene_builder.h"
#include "repo/lib/datastructure/repo_bounds.h"

#include <memory>

#include <ifcparse/IfcFile.h>
#include <ifcparse/Ifc2x3.h>
#include <ifcgeom/Iterator.h>
#include <ifcgeom/IfcGeomElement.h>
#include <ifcgeom/ConversionSettings.h>

#define IfcSchema Ifc2x3

namespace repo {
	namespace manipulator {
		namespace modelconvertor {
			namespace ifcHelper {

				/*
				* The RepoIfcSerialiser behaves similarly to a GeometrySerialiser of IfcOpenShell.
				* It processes a filtered list of elements, and builds the tree around them (as
				* oppposed to performing a forward traversal of the tree).
				*
				*/
				class IFCSerialiser
				{
				public:
					IFCSerialiser(IfcParse::IfcFile& file, repo::manipulator::modelutility::RepoSceneBuilder* builder);

				private:
					IfcParse::IfcFile& file;
					ifcopenshell::geometry::Settings settings;
					std::string kernel;
					int numThreads;
					repo::manipulator::modelutility::RepoSceneBuilder* builder;
					repo::lib::RepoUUID rootNodeId;
					std::unordered_map<std::string, repo::lib::repo_material_t> materials;
					std::unordered_map<int64_t, repo::lib::RepoUUID> sharedIds;

				public:
					/*
					* Gets the bounds from the file and updates the scene builder and importer
					* settings so all geometry is imported with the min world offset.
					*/
					void updateBounds();

					/*
					* Import all elements into the RepoSceneBuilder
					*/
					void import();

				private:

					void createRootNode();

					void configureSettings();
					
					repo::lib::RepoBounds getBounds();
					
					const repo::lib::repo_material_t& resolveMaterial(const ifcopenshell::geometry::taxonomy::style::ptr ptr);

					/*
					* Creates the MeshNode(s) for a given element - this may consist of a
					* number of primitive sets, with multiple materials. Will also create
					* the tree and metadata nodes.
					*/
					void import(const IfcGeom::TriangulationElement* triangulation);

					/*
					* Given an arbitrary Ifc Object, determine from its relationships which is
					* the best one for it so it under in the acyclic tree.
					*/
					const IfcSchema::IfcObjectDefinition* getParent(const IfcSchema::IfcObjectDefinition* object);

					/*
					* Creates the tree entry for the object, and recursively all its parents
					* upwards towards the project root.
					* 
					* How the tree is built depends on the object's own type and the
					* relationships available to it.
					* 
					* Objects are things that semantically exist, but are not necessarily
					* present in the physical world - that is, they could include walls, doors,
					* but also storeys and projects. Both types of object coexist in the tree.
					*
					*/
					repo::lib::RepoUUID createTransformationNode(const IfcSchema::IfcObjectDefinition* object);

					/*
					* Creates a transformation node under the parent object to group together
					* entities of a given type.
					*/
					repo::lib::RepoUUID createTransformationNode(
						const IfcSchema::IfcObjectDefinition* parent, 
						const IfcParse::entity& type);

					std::unique_ptr<repo::core::model::MetadataNode> createMetadataNode(const IfcSchema::IfcObjectDefinition* object);

					/*
					* If this entity sit under an empty node in the tree that doesn't exist in
					* the Ifc, but exists in the Repo tree to group entities by type.
					*/
					bool shouldGroupByType(const IfcSchema::IfcObjectDefinition* object, const IfcSchema::IfcObjectDefinition* parent);

					/*
					* Builds metadata and possibly transformation nodes for this element. If
					* the caller passes false to createTransform, then it is expected any mesh
					* nodes will attach directly to the parents.
					*/
					repo::lib::RepoUUID getParentId(const IfcGeom::Element* element, bool createTransform);
				};

			}
		}
	}
}