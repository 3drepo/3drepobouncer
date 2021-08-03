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


#include "c_sharp_wrapper.h"
#include <repo/core/model/bson/repo_node_material.h>
#include <repo/core/model/bson/repo_node_texture.h>
#include <repo/lib/repo_exception.h>
#include <boost/filesystem.hpp>
#include <fstream>

using namespace repo::lib;

const static int MAJOR_NUMBER = 4;
const static int MINOR_NUMBER = 17;
const static int MINOR_REV = 1;


CSharpWrapper* CSharpWrapper::wrapper = nullptr;
CSharpWrapper::CSharpWrapper()
	: controller(new repo::RepoController()),
	token(nullptr),
	scene(nullptr),
	vrEnabled(false)
{
}

CSharpWrapper::~CSharpWrapper()
{
	if (controller)
	{
		if (token)
			controller->destroyToken(token);
		if (scene)
			delete scene;
		delete controller;
	}

}

bool CSharpWrapper::connect(
	const std::string &config
)
{
	std::string msg;
	try {
		token = controller->init(msg, repo::lib::RepoConfig::fromFile(config));
		return token;
	} catch(const repo::lib::RepoException &exception) {
		return false;
	}
}

void CSharpWrapper::freeSuperMesh(
		const int &index)
{

	if (superMeshes.size() > index && superMeshes[index] != nullptr)
	{
		superMeshes[index] = nullptr;
	}
}

void CSharpWrapper::getIdMapBuffer(
	const int &index,
	const int &smIndex,
	float* &idMap) const
{
	if (idMapBuf.size() > index && idMapBuf[index].size() > smIndex)
	{
		memcpy(idMap, idMapBuf[index][smIndex].data(), sizeof(*idMap) * idMapBuf[index][smIndex].size());
	}
}

CSharpWrapper* CSharpWrapper::getInstance()
{
	if (!wrapper)
		wrapper = new CSharpWrapper();
	return wrapper;
}

void CSharpWrapper::getMaterialInfo(
	const std::string &materialID,
	float* &diffuse,
	float* &specular,
	float* &shininess,
	float* &shininessStrength) const
{
	auto it = matNodes.find(materialID);
	if (it != matNodes.end())
	{
		*shininess = 0;
		*shininessStrength = 1;
		if (it->second.shininess == it->second.shininess)
			*shininess = it->second.shininess;
		if (it->second.shininessStrength == it->second.shininessStrength)
			*shininessStrength = it->second.shininessStrength;

		for (size_t i = 0; i < 3; i++)
		{
			diffuse[i] = 0.5;
			specular[i] = 0;
		}

		for (size_t i = 0; i < it->second.diffuse.size(); i++)
		{
			diffuse[i] = it->second.diffuse[i];
		}

		for (size_t i = 0; i < it->second.specular.size(); i++)
		{
			specular[i] = it->second.specular[i];
		}

		diffuse[3] = it->second.opacity;
	}
}

std::string CSharpWrapper::getOrgMeshInfoFromSubMesh(
	const int &index,
	const int &smIndex,
	const int &orgMeshIdx,
	int* &smFFrom,
	int* &smFTo) const
{
	std::string matID;
	if (meshMappings.size() > index &&
		meshMappings[index].size() > smIndex &&
		meshMappings[index][smIndex].size() > orgMeshIdx)
	{
		*smFFrom = meshMappings[index][smIndex][orgMeshIdx].triFrom;
		*smFTo = meshMappings[index][smIndex][orgMeshIdx].triTo;
		matID = meshMappings[index][smIndex][orgMeshIdx].material_id.toString();
	}

	return matID;
}

int CSharpWrapper::getNumSuperMeshes() const
{
	return superMeshes.size();
}

std::string CSharpWrapper::getRevisionList(
	const std::string &database,
	const std::string &project)
{
	auto revs = controller->getAllFromCollectionContinuous(token, database, project + ".history");
	repoTrace << "Trying to find revisions for " << database << "." << project;
	std::stringstream ss;
	bool first = true;
	for(const auto rev : revs)
	{
		auto id = repo::core::model::RepoNode(rev).getUniqueID().toString();
		if(first)
		{
			ss << id ;
			first = false;
		}
		else
		{
			ss << "," << id;
		}
	}

	return ss.str();
}


void CSharpWrapper::getSuperMeshBuffers(
	const int &index,
	float* &vertices,
	float* &normals,
	uint16_t* &faces,
	float* &uvs,
	int* &numOrgMeshes) const
{
	if (superMeshes.size() > index)
	{
		auto vBuf = superMeshes[index]->getVertices();
		size_t vByteSize = sizeof(*vBuf.data()) * vBuf.size();
		memcpy(vertices, vBuf.data(), vByteSize);
		auto normalsBuf = superMeshes[index]->getNormals();
		if(normalsBuf.size())
			memcpy(normals, normalsBuf.data(), vByteSize);
		memcpy(faces, serialisedFaceBuf[index].data(), sizeof(*serialisedFaceBuf[index].data()) * serialisedFaceBuf[index].size());

		auto uvs_in = superMeshes[index]->getUVChannelsSeparated();
		if (uvs_in.size())
		{
			memcpy(uvs, uvs_in[0].data(), sizeof(*uvs_in[0].data()) * uvs_in[0].size());
		}

		for (int i = 0; i < meshMappings[index].size(); ++i)
		{
			numOrgMeshes[i] = meshMappings[index][i].size();
		}

	}
}

void CSharpWrapper::getSubMeshInfo(const int &index,
	const int &subIdx,
	int32_t* &vFrom,
	int32_t* &vTo,
	int32_t* &fFrom,
	int32_t* &fTo) const
{
	if (superMeshes.size() > index)
	{
		auto smMapping = superMeshes[index]->getMeshMapping();
		if (smMapping.size() > subIdx)
		{
			auto mapping = smMapping[subIdx];
			*vFrom = mapping.vertFrom;
			*vTo = mapping.vertTo;
			*fFrom = mapping.triFrom;
			*fTo = mapping.triTo;
		}
	}
}

std::string CSharpWrapper::getSuperMeshInfo(
	const int &index,
	long* &subMeshes,
	long* &numVertices,
	long* &numFaces,
	bool* &hasNormals,
	bool* &hasUV,
	int* &primitiveType,
	int* &numMappings) const
{
	std::string res;
	if (superMeshes.size() > index)
	{
		*subMeshes = superMeshes[index]->getMeshMapping().size();
		*numVertices = superMeshes[index]->getVertices().size();
		*numFaces = superMeshes[index]->getFaces().size();
		*hasUV = superMeshes[index]->getUVChannels().size();
		*hasNormals = superMeshes[index]->getNormals().size();
		*primitiveType = static_cast<int>(superMeshes[index]->getPrimitive());
		*numMappings = numIdMappings[index];
		res = superMeshes[index]->getUniqueID().toString();
	}

	return res;
}

void CSharpWrapper::getTexture(
	char* materialID,
	char* texBuf) const
{
	auto it = matToTexture.find(materialID);
	if (it != matToTexture.end())
	{
		memcpy(texBuf, it->second.data(), sizeof(*texBuf) * it->second.size());
	}
}

std::string CSharpWrapper::getVersion() const
{
	std::stringstream ss;
	ss << "v" << MAJOR_NUMBER << "." << MINOR_NUMBER << "." << MINOR_REV << " (Bouncer version: v" + controller->getVersion() << ")";
	return ss.str();
}

int CSharpWrapper::hasTexture(
	const std::string &materialID) const
{
	int res = -1;
	auto it = matToTexture.find(materialID);
	if (it != matToTexture.end())
	{
		res = it->second.size();
	}
	return res;
}

bool CSharpWrapper::initialiseSceneForAssetBundle(
	const std::string &database,
	const std::string &project,
	const std::string &revisionID)
{
	//clean up last job's data if available
	superMeshes.clear();
	jsonFiles.clear();
	serialisedFaceBuf.clear();
	idMapBuf.clear();
	meshMappings.clear();
	matNodes.clear();
	matToTexture.clear();
	numIdMappings.clear();

	if(scene)
		delete scene;

	if(revisionID.empty())
		scene = controller->fetchScene(token, database, project, REPO_HISTORY_MASTER_BRANCH, true,
				false, false, false, { repo::core::model::RevisionNode::UploadStatus::MISSING_BUNDLES });
	else
		scene = controller->fetchScene(token, database, project, revisionID, false,
				false, false, false, { repo::core::model::RevisionNode::UploadStatus::MISSING_BUNDLES });
	bool success  = false;
	if (scene)
	{
		projectName = project;
		databaseName = database;
		vrEnabled = controller->isVREnabled(token, scene);
		superMeshes = controller->initialiseAssetBuffer(token, scene, jsonFiles, unityAssets, serialisedFaceBuf, idMapBuf, meshMappings);
		success = superMeshes.size();

		// Get the total number of mappings contained in this supermesh. This is needed ahead of time to build things like textures.
		// We could consider passing this from the AssetModelExport in the future.

		for (auto splitMeshIdMap : idMapBuf) {
			numIdMappings.push_back(splitMeshIdMap.back().back() + 1);
		}

		//We dont' need the scene anymore
		for (auto mat : scene->getAllMaterials(repo::core::model::RepoScene::GraphType::OPTIMIZED))
		{
			auto matNode = dynamic_cast<repo::core::model::MaterialNode*>(mat);
			matNodes[mat->getUniqueID().toString()] = matNode->getMaterialStruct();
			auto textureChildren = scene->getChildrenNodesFiltered(
				repo::core::model::RepoScene::GraphType::OPTIMIZED,
				mat->getSharedID(),
				repo::core::model::NodeType::TEXTURE
				);
			if (textureChildren.size())
			{
				//There can only be 1 texture per material
				auto textNode = dynamic_cast<repo::core::model::TextureNode*>(textureChildren[0]);
				matToTexture[mat->getUniqueID().toString()] = textNode->getRawData();
			}
		}
		scene->clearStash();
	}

	return success;
}

bool CSharpWrapper::isVREnabled()
{
	return vrEnabled;
}

bool CSharpWrapper::isFederation(
				const std::string &database,
				const std::string &project) const
{
	auto tempScene = controller->fetchScene(token, database, project, REPO_HISTORY_MASTER_BRANCH, true, true);
	return tempScene && tempScene->getAllReferences(repo::core::model::RepoScene::GraphType::DEFAULT).size() > 0;
}

bool CSharpWrapper::saveAssetBundles(
	char** assetFiles,
	int      length
	)
{
	repo_web_buffers_t webBuffers;
	webBuffers.jsonFiles = jsonFiles;
	webBuffers.unityAssets = unityAssets;
	for (int i = 0; i < length; ++i)
	{
		std::ifstream inputStream;

		inputStream.open(assetFiles[i], std::ios::in | std::ios::binary);

		if (inputStream.is_open())
		{
			std::ostringstream ss;
			ss << inputStream.rdbuf();
			auto s = ss.str();
			boost::filesystem::path path(assetFiles[i]);
			std::string fileName = path.filename().string();
			fileName = "/" + databaseName + "/" + projectName + "/" + fileName;
			webBuffers.geoFiles[fileName] = std::vector<uint8_t>(s.begin(), s.end());
		}
		else
		{
			repoError << "Failed to find asset bundle";
			return false;
		}
	}

	return controller->commitAssetBundleBuffers(token, scene, webBuffers);
}
