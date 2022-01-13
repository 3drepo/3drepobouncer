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

#include "repo_csharp_interface.h"
#include "c_sharp_wrapper.h"


char* cStringCopy(const std::string& string)
{
	if (string.empty())
		return nullptr;

	char* p = (char*)malloc(sizeof(*p) * string.length());
	strcpy(p, string.c_str());

	return p;
}

bool repoConnect(
	char* configPath)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return wrapper->connect(configPath);
}

void repoFreeSuperMesh(
	int meshIndex)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	wrapper->freeSuperMesh(meshIndex);
}

void repoGetIdMapBuffer(int index, int smIndex, float* idMap)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	wrapper->getIdMapBuffer(index, smIndex, idMap);
}

void repoGetMaterialInfo(
	char* materialID,
	float* diffuse,
	float* specular,
	float* shininess,
	float* shininessStrength)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	wrapper->getMaterialInfo(materialID, diffuse, specular, shininess, shininessStrength);
}

char* repoGetOrgMeshInfoFromSubMesh(
	int index,
	int smIndex,
	int orgMeshIdx,
	int* smFFrom,
	int* smFTo)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return  cStringCopy(wrapper->getOrgMeshInfoFromSubMesh(index, smIndex, orgMeshIdx, smFFrom, smFTo));
}

int repoGetNumSuperMeshes()
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return wrapper->getNumSuperMeshes();
}

char* repoGetRevisionList(
	char* database,
	char* project)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return cStringCopy(wrapper->getRevisionList(database, project));
}

void repoGetSubMeshInfo(
	int index,
	int subIdx,
	int* vFrom,
	int* vTo,
	int* fFrom,
	int* fTo)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return wrapper->getSubMeshInfo(index, subIdx, vFrom, vTo, fFrom, fTo);
}

void repoGetSuperMeshBuffers(
	int index,
	float* vertices,
	float* normals,
	uint16_t* faces,
	float* uvs,
	int* numOrgMeshes)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	wrapper->getSuperMeshBuffers(index, vertices, normals, faces, uvs, numOrgMeshes);
}

char* repoGetSuperMeshInfo(
	int index,
	long* subMeshes,
	long* numVertices,
	long* numFaces,
	bool* hasNormals,
	bool* hasUV,
	int* primitiveType,
	int* numMappings)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return cStringCopy(wrapper->getSuperMeshInfo(index, subMeshes, numVertices, numFaces, hasNormals, hasUV, primitiveType, numMappings));
}

void repoGetTexture(
	char* materialID,
	char* texBuf)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return wrapper->getTexture(materialID, texBuf);
}

char* repoGetWrapperVersionString()
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return cStringCopy(wrapper->getVersion());
}

int repoHasTexture(
	char* materialID)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return wrapper->hasTexture(materialID);
}

bool repoIsVREnabled()
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return wrapper->isVREnabled();
}

bool repoIsFederation(
	char* database,
	char* project)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return wrapper->isFederation(database, project);
}

bool repoLoadSceneForAssetBundleGeneration(
	char* database,
	char* project,
	char* revisionID)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return wrapper->initialiseSceneForAssetBundle(database, project, revisionID);
}

bool repoSaveAssetBundles(
	char** assetFiles,
	int      length
)
{
	auto wrapper = repo::lib::CSharpWrapper::getInstance();
	return wrapper->saveAssetBundles(assetFiles, length);
}

void repoDeinit() {
	repo::lib::CSharpWrapper::destroyInstance();
}
