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
* Allows Export functionality from 3D Repo World to SRC
*/


#pragma once

#include <string>

#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>
#include "../../../lib/json_parser.h"

#include "repo_model_export_abstract.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{

			//see http://stackoverflow.com/questions/2855741/why-boost-property-tree-write-json-saves-everything-as-string-is-it-possible-to
			struct stringTranslator
			{
				typedef std::string internal_type;
				typedef std::string external_type;

				boost::optional<std::string> get_value(const std::string &v) { return  v.substr(1, v.size() - 2); }
				boost::optional<std::string> put_value(const std::string &v) { return '"' + v + '"'; }
			};

			class SRCModelExport : public AbstractModelExport
			{	
			public:
				/**
				* Default Constructor, export model with default settings
				* @param scene repo scene to convert
				*/
				SRCModelExport(const repo::core::model::RepoScene *scene);

				/**
				* Default Destructor
				*/
				virtual ~SRCModelExport();

				/**
				* Export a repo scene graph to file
				* @param filePath path to destination file
				* @return returns true upon success
				*/
				virtual bool exportToFile(
					const repo::core::model::RepoScene *scene,
					const std::string &filePath){
					return false;
				};

				/**
				* Return the SRC file as raw bytes buffer
				* returns an empty vector if the export has failed
				*/
				std::unordered_map<std::string, std::vector<uint8_t>> getFileAsBuffer();

				/**
				* Get supported file formats for this exporter
				*/
				static std::string getSupportedFormats();

				/**
				* Returns the status of the converter,
				* whether it has successfully converted the model
				* @return returns true if success
				*/
				bool isOk() const
				{
					return convertSuccess;
				}

				/**
				* @param filePath path to destination file (including file extension)
				* @return returns true upon success
				*/
				bool writeSceneToFile(
					const std::string &filePath);
				
			private:
				const repo::core::model::RepoScene *scene;
				bool convertSuccess;
				std::unordered_map<std::string, boost::property_tree::ptree> trees;
				repo::core::model::RepoScene::GraphType gType;
				std::unordered_map<std::string, std::vector<uint8_t>> fullDataBuffer;


				/**
				* Convert a Mesh Node into src format
				* @param mesh the mesh to convert
				* @param index a counter indiciating the mesh index
				* @param faceBuf face buffer with only the face indices
				* @param idMapBuf idMapping for each sub meshes
				* @param fileExt file extension required for this SRC file (*.src/.src.mpc/.src?<query>)
				*/
				void addMeshToExport(
					const repo::core::model::MeshNode &mesh,
					const size_t &idx,
					const std::vector<uint16_t> &faceBuf,
					const std::vector<std::vector<float>>  &idMapBuf,
					const std::string                      &fileExt
					);

				/**
				* Create a tree representation for the graph
				* This creates the header of the SRC
				*/
				bool generateTreeRepresentation();
				
				template <typename T>
				void addToTree(
					boost::property_tree::ptree &tree,
					const std::string           &label,
					const T                     &value)
				{
					if (label.empty())
						tree.put(label, value);
					else
						tree.add(label, value);
				}

				/**
				* Create a property tree with an array of ints
				* @param children 
				*/
				template <typename T>
				boost::property_tree::ptree createPTArray(std::vector<T> children)
				{					
					boost::property_tree::ptree arrayTree;
					for (const auto &child : children)
					{
						boost::property_tree::ptree childTree;
						addToTree(childTree, "", child);
						arrayTree.push_back(std::make_pair("", childTree));
					}


					return arrayTree;		
				}

			};

			// Template specialization
			template <>
			void SRCModelExport::addToTree<std::string>(
				boost::property_tree::ptree &tree,
				const std::string           &label,
				const std::string           &value);


		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo

