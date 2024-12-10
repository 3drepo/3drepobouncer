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
*  BSON factory that creates specific BSON objects.
*  This essentially functions as the builder (like Mongo BsonObjBuilder)
*/

#pragma once

#include "repo_bson_ref.h"
#include "repo_bson_sequence.h"
#include "repo_bson_task.h"
#include "repo_bson_assets.h"
#include "repo_bson_calibration.h"
#include "repo_node.h"
#include "repo_node_metadata.h"
#include "repo_node_material.h"
#include "repo_node_mesh.h"
#include "repo_node_supermesh.h"
#include "repo_node_reference.h"
#include "repo_node_model_revision.h"
#include "repo_node_texture.h"
#include "repo_node_transformation.h"

#include "repo/lib/datastructure/repo_variant.h"

namespace repo {
	namespace core {
		namespace model {
			class REPO_API_EXPORT RepoBSONFactory
			{
			public:
				/**
				* Create RepoRef
				* @param fileName name of the file - this is what is stored in the _id member, and should be a UUID or string.
				* @param type type of storage
				* @param link reference link
				* @param size size of file in bytes
				* @return returns a bson with this reference information
				*/

				template<typename IdType>
				static RepoRefT<IdType> makeRepoRef(
					const IdType &id,
					const RepoRef::RefType &type,
					const std::string &link,
					const uint32_t size,
					const RepoRef::Metadata& metadata = {});

				/**
				* Create a RepoBundles list BSON
				* @param revisionID uuid of the revision (default: master branch)
				* @param assets list of RepoBundles assets
				* @param database name of the database to reference
				* @param model model ID (string) to reference
				* @param offset world offset shift coordinates of the model
				* @param vrAssetFiles list of VR Unity assets
				* @param iosAssetFiles list of iOS Unity assets
				* @param androidAssetFiles list of android Unity assets
				* @param jsonFiles list of JSON files
				* @return returns a RepoAssets
				*/
				static RepoAssets makeRepoBundleAssets(
					const repo::lib::RepoUUID& revisionID,
					const std::vector<std::string>& repoBundleFiles,
					const std::string& database,
					const std::string& model,
					const std::vector<double>& offset,
					const std::vector<std::string>& repoJsonFiles,
					const std::vector<RepoSupermeshMetadata> metadata);

				/**
				* Create a Drawing Calibration BSON
				* @param projectId uuid of the project
				* @param drawingId uuid of the drawing
				* @param revisionId uuid of the revision
				* @param horizontal3d two reference points in the 3d space
				* @param horizontal2d two reference points in the 2d space
				* @param units the units used for the values.
				* @return returns a RepoCalibration
				*/
				static RepoCalibration makeRepoCalibration(
					const repo::lib::RepoUUID& projectId,
					const repo::lib::RepoUUID& drawingId,
					const repo::lib::RepoUUID& revisionId,
					const std::vector<repo::lib::RepoVector3D>& horizontal3d,
					const std::vector<repo::lib::RepoVector2D>& horizontal2d,
					const std::string& units
				);

				/*
				* -------------------- REPO NODES ------------------------
				*/

				/**
				* Create a Material Node
				* @param a struct that contains the info about the material
				* @param name of the Material
				* @return returns a Material node
				*/
				static MaterialNode makeMaterialNode(
					const repo_material_t &material,
					const std::string     &name = std::string(),
					const std::vector<repo::lib::RepoUUID> &parents = std::vector<repo::lib::RepoUUID>(),
					const int             &apiLevel = REPO_NODE_API_LEVEL_1);

				/**
				* Create a Metadata Node
				* @param keys labels for the fields
				* @param values values of the fields, matching the key parameter
				* @param name Name of Metadata (optional)
				* @param parents
				* @param apiLevel Repo Node API level (optional)
				* @return returns a metadata node
				*/
				static MetadataNode makeMetaDataNode(
					const std::vector<std::string>  &keys,
					const std::vector<repo::lib::RepoVariant>  &values,
					const std::string               &name = std::string(),
					const std::vector<repo::lib::RepoUUID>     &parents = std::vector<repo::lib::RepoUUID>(),
					const int                       &apiLevel = REPO_NODE_API_LEVEL_1);

				/**
				* Create a Metadata Node
				* @param data list of key value pair
				* @param name Name of Metadata (optional)
				* @param parents
				* @param apiLevel Repo Node API level (optional)
				* @return returns a metadata node
				*/
				static MetadataNode makeMetaDataNode(
					const std::unordered_map<std::string, repo::lib::RepoVariant>  &data,
					const std::string            &name = std::string(),
					const std::vector<repo::lib::RepoUUID> &parents = std::vector<repo::lib::RepoUUID>(),
					const int                    &apiLevel = REPO_NODE_API_LEVEL_1);

				/**
				* Create a Mesh Node
				* @param vertices vector of vertices
				* @param faces vector of faces
				* @param normals vector of normals
				* @param boundingBox vector of 2 vertex indicating the bounding box
				* @param uvChannels vector of UV Channels
				* @param colors vector of colours
				* @param outline outline generated from bounding box
				* @param name name of the node (optional, default empty string)
				* @param apiLevel API level of the node (optional, default REPO_NODE_API_LEVEL_1)
				* @return returns a mesh node
				*/
				static MeshNode makeMeshNode(
					const std::vector<repo::lib::RepoVector3D>& vertices,
					const std::vector<repo_face_t>& faces,
					const std::vector<repo::lib::RepoVector3D>& normals,
					const repo::lib::RepoBounds& boundingBox,
					const std::vector<std::vector<repo::lib::RepoVector2D>>& uvChannels = {},
					const std::string& name = std::string(),
					const std::vector<repo::lib::RepoUUID>& parents = {});

				static SupermeshNode makeSupermeshNode(
					const std::vector<repo::lib::RepoVector3D>& vertices,
					const std::vector<repo_face_t>& faces,
					const std::vector<repo::lib::RepoVector3D>& normals,
					const repo::lib::RepoBounds& boundingBox,
					const std::vector<std::vector<repo::lib::RepoVector2D>>& uvChannels,
					const std::string& name,
					const std::vector<repo_mesh_mapping_t>& mappings
				);

				static SupermeshNode makeSupermeshNode(
					const std::vector<repo::lib::RepoVector3D>& vertices,
					const std::vector<repo_face_t>& faces,
					const std::vector<repo::lib::RepoVector3D>& normals,
					const repo::lib::RepoBounds& boundingBox,
					const std::vector<std::vector<repo::lib::RepoVector2D>>& uvChannels,
					const std::vector<repo_mesh_mapping_t>& mappings,
					const repo::lib::RepoUUID& id,
					const repo::lib::RepoUUID& sharedId,
					const std::vector<float> mappingIds = {}
				);

				/**
				* Create a Reference Node
				* If revision ID is unique, it will be referencing a specific revision
				* If it isn't unique, it will represent a branch ID and reference its head
				* @param database name of the database to reference
				* @param project name of the project to reference
				* @param revisionID uuid of the revision (default: master branch)
				* @param isUniqueID is revisionID a specific revision ID (default: false - i.e it is a branch ID)
				* @return returns a reference node
				*/
				static ReferenceNode makeReferenceNode(
					const std::string &database,
					const std::string &project,
					const repo::lib::RepoUUID    &revisionID = repo::lib::RepoUUID(REPO_HISTORY_MASTER_BRANCH),
					const bool        &isUniqueID = false,
					const std::string &name = std::string(),
					const int         &apiLevel = REPO_NODE_API_LEVEL_1);

				/**
				* Create a Revision Node
				* @param user name of the user who is commiting this project
				* @param branch UUID of the branch
				* @param currentNodes vector of current nodes (unique IDs)
				//* @param added    vector of what nodes are added    (shared IDs)
				//* @param removed  vector of what nodes are deleted  (shared IDs)
				//* @param modified vector of what nodes are modified (shared IDs)
				* @param files    vector of the original files for this model
				* @param parent   UUID of parent (in a vector)
				* @param worldOffset x,y,z offset for world coordinates
				* @param message  A message to describe what this commit is for (optional)
				* @param tag      A tag for this specific revision (optional)
				* @param apiLevel API Level(optional)
				* @return returns a texture node
				*/

				static ModelRevisionNode makeRevisionNode(
					const std::string			   &user,
					const repo::lib::RepoUUID                 &branch,
					const repo::lib::RepoUUID                 &revId,
					const std::vector<std::string> &files = std::vector<std::string>(),
					const std::vector<repo::lib::RepoUUID>    &parent = std::vector<repo::lib::RepoUUID>(),
					const std::vector<double>       &worldOffset = std::vector<double>(),
					const std::string              &message = std::string(),
					const std::string              &tag = std::string(),
					const int                      &apiLevel = REPO_NODE_API_LEVEL_1
				);

				/**
				* Create a Texture Node
				* @param name name of the texture node, with extension to indicate file format
				* @param data binary data to store
				* @param byteCount size of the binary data in bytes
				* @param width width of the texture
				* @param height height of the texture
				* @param apiLevel API Level(optional)
				* @return returns a texture node
				*/
				static TextureNode makeTextureNode(
					const std::string &name,
					const char        *data,
					const uint32_t    &byteCount,
					const uint32_t    &width,
					const uint32_t    &height,
					const std::vector<repo::lib::RepoUUID>& parentIDs = std::vector<repo::lib::RepoUUID>(),
					const int         &apiLevel = REPO_NODE_API_LEVEL_1);

				/**
				* Create a Transformation Node
				* @param transMatrix a 4 by 4 transformation matrix (optional - default is identity matrix)
				* @param name name of the transformation(optional)
				* @param parents vector of parents uuid for this node (optional)
				* @param apiLevel API Level(optional)
				* @return returns a Transformation node
				*/
				static TransformationNode makeTransformationNode(
					const repo::lib::RepoMatrix &transMatrix = repo::lib::RepoMatrix(),
					const std::string                     &name = "<transformation>",
					const std::vector<repo::lib::RepoUUID>		      &parents = std::vector<repo::lib::RepoUUID>(),
					const int                             &apiLevel = REPO_NODE_API_LEVEL_1);

				static RepoSequence makeSequence(
					const std::vector<repo::core::model::RepoSequence::FrameData> &frameData,
					const std::string &name,
					const repo::lib::RepoUUID &id,
					const uint64_t firstFrame,
					const uint64_t lastFrame
				);

				static RepoTask makeTask(
					const std::string &name,
					const uint64_t &startTime,
					const uint64_t &endTime,
					const repo::lib::RepoUUID &sequenceID,
					const std::unordered_map<std::string, std::string>  &data,
					const std::vector<repo::lib::RepoUUID> &resources,
					const repo::lib::RepoUUID &parent = repo::lib::RepoUUID::createUUID(),
					const repo::lib::RepoUUID &id = repo::lib::RepoUUID::createUUID()
				);
			};
		} //namespace model
	} //namespace core
} //namespace repo
