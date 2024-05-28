/**
*  Copyright (C) 2016 3D Repo Ltd
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
* The MeshMapReorganiser combines or splits the mappings in a MeshNode such
* that all the mappings come as close to the vertThreshold and faceThreshold
* limits as possible, but never exceed them. The result is a new MeshNode with
* a new set of mappings.
*
* The faces delineated by the new mappings are re-indexed so they refer directly
* to vertices between vFrom and vTo of any mapping (though they are stored still
* in a shared array).
*
* Using MeshMapReorganiser is destructive to the original mappings, however the
* Ids are unchanged, and so can be used to index directly into the original
* MeshNode's mappings. Additionally, getSplitMapping() returns a map from the
* mesh_ids in the original mappings to the (indices of) the new one(s) that
* contain it, and getMappingsPerSubMesh() returns for each new mapping, a list
* of the original mappings that are encompassed by them.
*/

#pragma once
#include <unordered_map>
#include "../../core/model/bson/repo_node_mesh.h"

namespace repo {
	namespace manipulator {
		namespace modelutility {
			class MeshMapReorganiser
			{
			public:
				/**
				* Construct a mesh reorganiser
				* if the reorganiser failed to generate
				* a remapped mesh, it will return an empty meshNode.
				* @param mesh the mesh to reorganise
				* @param vertThreshold maximum vertices
				*/
				MeshMapReorganiser(
					const repo::core::model::SupermeshNode	*mesh,
					const size_t                        &vertThreshold,
					const size_t						&faceThreshold);
				~MeshMapReorganiser();

				/**
				* Return idMap arrays of the modified mesh (for material mapping)
				* @return returns a idMap arrays
				*/
				std::vector<std::vector<float>> getIDMapArrays() const;

				/**
				* Get all mesh mapping, grouped by the sub meshes they belong to
				* @return a vector (new submeshes) of vector of mesh mappings(original submeshes)
				*/
				std::vector<std::vector<repo_mesh_mapping_t>> getMappingsPerSubMesh() const;

				/**
				* Get the mesh, with mesh mappings and buffers modified
				* @return returns the modified mesh
				*/
				repo::core::model::SupermeshNode getRemappedMesh() const;

				/**
				* Return serialised faces of the modified mesh
				* @return returns a serialised buffer of faces
				*/
				std::vector<uint16_t> getSerialisedFaces() const;

				/**
				* Get the mapping between submeshes UUID to new super mesh index
				* @return sub mesh to super mesh(es) mapping
				*/
				std::unordered_map<repo::lib::RepoUUID, std::vector<uint32_t>, repo::lib::RepoUUIDHasher> getSplitMapping() const;

			private:
				/**
				* Complete a submesh by filling in the ending parts of  mesh mapping
				* @param mapping mapping to fill in
				* @param nVertices number of vertices in this mapping
				* @param nFaces number of faces in this mapping
				* @param minBox minimum bounding box (length of 3)
				* @param maxBox maximum bounding box (length of 3)
				*/
				void finishSubMesh(
					repo_mesh_mapping_t &mapping,
					std::vector<float> &minBox,
					std::vector<float> &maxBox,
					const size_t &nVertices,
					const size_t &nFaces
				);

				void newMatMapEntry(
					const repo_mesh_mapping_t &mapping,
					const size_t        &sVertices,
					const size_t        &sFaces
				);

				void completeLastMatMapEntry(
					const size_t        &eVertices,
					const size_t        &eFaces,
					const std::vector<float>  &minBox = std::vector<float>(),
					const std::vector<float>  &maxBox = std::vector<float>()
				);

				/**
				* The beginning function for the whole process.
				* Merge or split submeshes where appropriate
				*/
				bool performSplitting();

				/**
				* Split a single large sub mesh that exceeds the number of
				* vertices into multiple sub meshes
				* @param currentSubMesh current sub mesh's mapping
				* @param newMappings current load of new mappings (consume and update)
				* @param idMapIdx idMap index value           (consume and update)
				* @param orgFaceIdx current face index        (consume and update)
				* @param totalVertexCount total vertice count (consume and update)
				* @param totalFaceCount total face count      (consume and update)
				*/
				bool splitLargeMesh(
					const repo_mesh_mapping_t        &currentSubMesh,
					std::vector<repo_mesh_mapping_t> &newMappings,
					size_t                           &idMapIdx,
					size_t                           &orgFaceIdx,
					size_t                           &totalVertexCount,
					size_t                           &totalFaceCount);

				/**
				* Start a new sub mesh
				* @param mapping mapping to fill in
				* @param meshID mesh ID
				* @param matID material ID
				* @param sVertices starting vertice #
				* @param sFaces    starting face #
				*/
				void startSubMesh(
					repo_mesh_mapping_t &mapping,
					const repo::lib::RepoUUID      &meshID,
					const repo::lib::RepoUUID      &sharedID,
					const repo::lib::RepoUUID      &matID,
					const size_t        &sVertices,
					const size_t        &sFaces
				);

				/**
				* Update the given bounding boxes given the current sub mesh's bounding boxes
				* @param min overall min bounding box to update
				* @param max overall max bounding box to update
				* @param smMin sub mesh bounding box min
				* @param smMin sub mesh bounding box max
				*/
				void updateBoundingBoxes(
					std::vector<float>  &min,
					std::vector<float>  &max,
					const repo::lib::RepoVector3D &smMin,
					const repo::lib::RepoVector3D &smMax);

				/**
				* Update the current ID Map array with the given values
				* @param n number of entires
				* @param value input value
				*/
				void updateIDMapArray(
					const size_t &n,
					const size_t &value);

				bool reMapSuccess;

				const repo::core::model::SupermeshNode *mesh;
				const size_t maxVertices;
				const size_t maxFaces;
				const std::vector<repo::lib::RepoVector3D> oldVertices;
				const std::vector<repo::lib::RepoVector3D> oldNormals;
				const std::vector<std::vector<repo::lib::RepoVector2D>> oldUVs;
				const std::vector<repo_face_t>   oldFaces;
				const std::vector<repo_color4d_t>   oldColors;

				std::vector<repo::lib::RepoVector3D> newVertices;
				std::vector<repo::lib::RepoVector3D> newNormals;
				std::vector<repo_face_t>   newFaces;
				std::vector<repo_color4d_t>   newColors;
				std::vector<std::vector<repo::lib::RepoVector2D>> newUVs;

				std::vector<uint16_t> serialisedFaces;

				std::vector<std::vector<float>> idMapBuf;
				std::unordered_map<repo::lib::RepoUUID, std::vector<uint32_t>, repo::lib::RepoUUIDHasher> splitMap;
				std::vector<std::vector<repo_mesh_mapping_t>> matMap;
				std::vector<repo_mesh_mapping_t> reMappedMappings;
			};
		}
	}
}
