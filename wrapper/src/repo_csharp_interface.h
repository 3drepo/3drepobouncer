/**
*  Copyright (C) 2017 3D Repo Ltd
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
*  Copyright (C) 2017 3D Repo Ltd
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
#include "c_sharp_wrapper.h"
/*
* Static, non object-orientated interface to connect to c#
* State information is stored in a singleton object CSharpWrapper
*/
#include <repo/repo_bouncer_global.h>

#if defined(_WIN32) || defined(_WIN64)
#   define REPO_WRAPPER_DECL_EXPORT __declspec(dllexport)
#   define REPO_WRAPPER_DECL_IMPORT __declspec(dllimport)
#else
#   define REPO_WRAPPER_DECL_EXPORT
#   define REPO_WRAPPER_DECL_IMPORT
#endif

//------------------------------------------------------------------------------
#if defined(REPO_WRAPPER_API_LIBRARY)
#   define REPO_WRAPPER_API_EXPORT REPO_WRAPPER_DECL_EXPORT
#else
#   define REPO_WRAPPER_API_EXPORT REPO_WRAPPER_DECL_IMPORT
#endif

extern "C"
{
	char* cStringCopy(const std::string &string)
	{
		if (string.empty())
			return nullptr;

		char* p = (char*)malloc(sizeof(*p) * string.length());
		strcpy(p, string.c_str());

		return p;
	}

	/**
	* Connect to a mongo database, authenticate by the admin database
	* @param configPath path to configuration file
	* @return returns true if successfully connected, false otherwise
	*/
	REPO_WRAPPER_API_EXPORT bool repoConnect(
		char* configPath);

	/*
	 * Free memory occupied by the super mesh at a given index
	 * @param meshIndex index of the super mesh to free
	 */
	REPO_WRAPPER_API_EXPORT void repoFreeSuperMesh(
		int meshIndex);

	/**
	* Get ID Map buffer for submesh
	* @param index supermesh index
	* @param smIndex submesh index
	* @param idMap pre-allocated buffer for the ID Map
	*/
	REPO_WRAPPER_API_EXPORT void repoGetIdMapBuffer(int index, int smIndex, float* idMap);

	/**
	* Retrieve the information of the materials given the material ID
	* @param materialID ID of the material
	* @param diffuse (output) pre-allocated array of 4 floats for diffuse colour
	* @param diffuse (output) pre-allocated array of 4 floats for specular colour
	* @param shiniess (output) shininess value
	* @param shiniess strength (output) shininess strength value
	*/
	REPO_WRAPPER_API_EXPORT void repoGetMaterialInfo(
		char* materialID,
		float* diffuse,
		float* specular,
		float* shininess,
		float* shininessStrength);

	/**
	* Get original mesh information from submesh
	* @param index supermesh Index
	* @param smIndex sub mesh index
	* @param orgMesh index within the sub mesh
	* @param smFfrom (output) starting index of faces
	* @param smFto   (output) ending index of faces
	*/
	REPO_WRAPPER_API_EXPORT char* repoGetOrgMeshInfoFromSubMesh(
		int index,
		int smIndex,
		int orgMeshIdx,
		int* smFFrom,
		int* smFTo);

	/**
	* Get number of super meshes in the loaded project
	* @return returns the number of super meshes in this project
	*/
	REPO_WRAPPER_API_EXPORT int repoGetNumSuperMeshes();

	/**
	 * Get number of revisions within the given project
	 * @param database name of database
	 * @param project name of project
	 * @return returns the number of revisions within this project
	 */
	REPO_WRAPPER_API_EXPORT char* repoGetRevisionList(
		char* database,
		char* project);

	/**
	* Retrieve location of buffers for this supermesh submesh
	* @param index supermesh index
	* @param subIdx submesh index
	* @param vFrom  (output) start index of vertices
	* @param vTo  (output) end index of vertices
	* @param fFrom  (output) start index of faces
	* @param fTo  (output) end index of faces
	*/
	REPO_WRAPPER_API_EXPORT void repoGetSubMeshInfo(
		int index,
		int subIdx,
		int* vFrom,
		int* vTo,
		int* fFrom,
		int* fTo);

	/**
	* Copy over buffers from superMesh
	* @param index the index of the supermesh within the supermesh array
	* @param vertices pre-allocated vertice buffer
	* @param normals pre-allocated normal buffer
	* @param faces pre-allocated face buffer
	* @param numOrgMeshes number of org meshes per submesh
	*/
	REPO_WRAPPER_API_EXPORT void repoGetSuperMeshBuffers(
		int index,
		float* vertices,
		float* normals,
		uint16_t* faces,
		float* uvs,
		int* numOrgMeshes);

	/**
	* Get info on a supermesh
	* @param index the index of the supermesh within the supermesh array
	* @param subMeshes number of sub meshes within this mesh
	* @param numVertices (output) the number of vertices
	* @param numFaces (output) number of faces
	* @param hasNormals (output) return true if it has Normals
	* @param hasUV (output) return true if it has UVs
	* @return return the unique ID of the supermesh as a string
	*/
	REPO_WRAPPER_API_EXPORT char* repoGetSuperMeshInfo(
		int index,
		long* subMeshes,
		long* numVertices,
		long* numFaces,
		bool* hasNormals,
		bool* hasUV,
		int* primitiveType,
		int* numMappings);

	/**
	* Get texture binary for the given material
	* @param materialID Unique ID of the material as a string
	* @param texBuf preallocated buffer to copy texture buffer into
	*/
	REPO_WRAPPER_API_EXPORT void repoGetTexture(
		char* materialID,
		char* texBuf);

	REPO_WRAPPER_API_EXPORT char* repoGetWrapperVersionString();

	/**
	* Check if the material has textures
	* @param materialID Unique ID of the material as a string
	* @return returns size of texture, 0 if none
	*/
	REPO_WRAPPER_API_EXPORT int repoHasTexture(
		char* materialID);

	/**
	 * Check if VR is enabled for this model
	 * @return returns true if it is enabled, false otherwise
	 */
	REPO_WRAPPER_API_EXPORT bool repoIsVREnabled();

	/**
	* Check if thegiven model is a federation of models
	* @param database name of the database
	* @param project name of the project
	* @return returns true if it is a federation, false otherwise
	*/
	REPO_WRAPPER_API_EXPORT bool repoIsFederation(
		char* database,
		char* project);

	/**
	* Load a Repo scene from the database, in preparation for asset bundle generation
	* @param database database where the project is
	* @param project name of the project
	* @param revisionID revision ID to load (if empty string, will load head of master)
	* @return returns true if successfully loaded the scene, false otherwise
	*/
	REPO_WRAPPER_API_EXPORT bool repoLoadSceneForAssetBundleGeneration(
		char* database,
		char* project,
		char* revisionID);

	/**
	* Save asset bundles into the database
	* @param assetfiles array of file paths to assets
	* @param length length of the assetfiles array
	*/
	REPO_WRAPPER_API_EXPORT bool repoSaveAssetBundles(
		char** assetFiles,
		int      length
	);
}
