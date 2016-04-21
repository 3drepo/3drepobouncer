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

#include "repo_model_export_web.h"
#include "../../../lib/repo_property_tree.h"
#include "../../../core/model/collection/repo_scene.h"

namespace repo{
	namespace manipulator{
		namespace modelconvertor{
			class SRCModelExport : public WebModelExport
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
				* Export all necessary files as buffers
				* @return returns a repo_src_export_t containing all files needed for this
				*          model to be rendered
				*/
				repo_web_buffers_t getAllFilesExportedAsBuffer() const;

			private:
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
				* Generate JSON mapping for multipart meshes
				* And add it into the object
				* @param mesh mesh to generate with
				* @param scene scene for reference
				* @param splitMapping how the mapping is split after subMesh split
				*/
				bool generateJSONMapping(
					const repo::core::model::MeshNode *mesh,
					const repo::core::model::RepoScene *scene,
					const std::unordered_map<repoUUID, std::vector<uint32_t>, RepoUUIDHasher> &splitMapping);

				/**
				* Create a tree representation for the graph
				* This creates the header of the SRC
				* @return returns true upon success
				*/
				bool generateTreeRepresentation();

				/**
				* Return the SRC file as raw bytes buffer
				* returns an empty vector if the export has failed
				*/
				std::unordered_map<std::string, std::vector<uint8_t>> getSRCFilesAsBuffer() const;
			};
		} //namespace modelconvertor
	} //namespace manipulator
} //namespace repo
