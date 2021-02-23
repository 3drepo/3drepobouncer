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
* Allows Export functionality from 3D Repo World to asset bundle
*/

#include "repo_model_export_asset.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../lib/repo_log.h"

using namespace repo::manipulator::modelconvertor;
const static size_t ASSET_MAX_VERTEX_LIMIT = 65535;
const static size_t ASSET_MAX_TRIANGLE_LIMIT = SIZE_MAX;
const static size_t ASSET_MAX_LINE_LIMIT = (65536 / 4);
//Labels for multipart JSON descriptor files
const static std::string MP_LABEL_APPEARANCE = "appearance";
const static std::string MP_LABEL_MAT_DIFFUSE = "diffuseColor";
const static std::string MP_LABEL_MAT_EMISSIVE = "emissiveColor";
const static std::string MP_LABEL_MATERIAL = "material";
const static std::string MP_LABEL_MAPPING = "mapping";
const static std::string MP_LABEL_MAT_SHININESS = "shininess";
const static std::string MP_LABEL_MAT_SPECULAR = "specularColor";
const static std::string MP_LABEL_MAT_TRANSPARENCY = "transparency";
const static std::string MP_LABEL_MAX = "max";
const static std::string MP_LABEL_MAX_GEO_COUNT = "maxGeoCount";
const static std::string MP_LABEL_MIN = "min";
const static std::string MP_LABEL_NAME = "name";
const static std::string MP_LABEL_NUM_IDs = "numberOfIDs";
const static std::string MP_LABEL_USAGE = "usage";

const static std::string MP_LABEL_ASSETS = "assets";
const static std::string MP_LABEL_VR_ASSETS = "vrAssets";
const static std::string MP_LABEL_IOS_ASSETS = "iosAssets";

const static std::string MP_LABEL_SHARED = "sharedID";
const static std::string MP_LABEL_JSONS = "jsonFiles";
const static std::string MP_LABEL_OFFSET = "offset";
const static std::string MP_LABEL_DATABASE = "database";
const static std::string MP_LABEL_PROJECT = "model";

AssetModelExport::AssetModelExport(
	const repo::core::model::RepoScene *scene,
	const bool vrEnabled
) : WebModelExport(scene),
generateVR(vrEnabled)
{
	//Considering all newly imported models should have a stash graph, we only need to support stash graph?
	if (convertSuccess)
	{
		if (gType == repo::core::model::RepoScene::GraphType::OPTIMIZED)
		{
			convertSuccess = generateTreeRepresentation();
		}
		else  if (!(convertSuccess = !scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT).size()))
		{
			repoError << "Scene has no optimised graph and it is not a federation graph. SRC Exporter relies on this.";
		}
	}
	else
	{
		repoError << "Unable to export to SRC : Empty scene graph!";
	}
}

AssetModelExport::~AssetModelExport()
{
}

repo_web_buffers_t AssetModelExport::getAllFilesExportedAsBuffer() const
{
	return {
		std::unordered_map<std::string, std::vector<uint8_t>>(),
		getJSONFilesAsBuffer(),
		getUnityAssets()
	};
}

repo::core::model::RepoUnityAssets AssetModelExport::getUnityAssets() const
{
	return unityAssets;
}

bool AssetModelExport::generateJSONMapping(
	const repo::core::model::MeshNode  *mesh,
	const repo::core::model::RepoScene *scene,
	const std::unordered_map<repo::lib::RepoUUID, std::vector<uint32_t>, repo::lib::RepoUUIDHasher> &splitMapping)
{
	bool success;
	if (success = mesh)
	{
		repo::lib::PropertyTree jsonTree;
		std::vector<repo_mesh_mapping_t> mappings = mesh->getMeshMapping();
		std::sort(mappings.begin(), mappings.end(),
			[](repo_mesh_mapping_t const& a, repo_mesh_mapping_t const& b) { return a.vertFrom < b.vertFrom; });

		size_t mappingLength = mappings.size();

		jsonTree.addToTree(MP_LABEL_NUM_IDs, mappingLength);
		jsonTree.addToTree(MP_LABEL_MAX_GEO_COUNT, mappingLength);

		std::vector<repo::lib::PropertyTree> mappingTrees;
		std::string meshUID = mesh->getUniqueID().toString();
		//Could get the mesh split function to pass a mapping out so we don't do this again.
		for (size_t i = 0; i < mappingLength; ++i)
		{
			auto mapIt = splitMapping.find(mappings[i].mesh_id);
			if (mapIt != splitMapping.end())
			{
				for (const uint32_t &subMeshID : mapIt->second)
				{
					repo::lib::PropertyTree mappingTree;

					mappingTree.addToTree(MP_LABEL_NAME, mappings[i].mesh_id.toString());

					repo::lib::RepoUUID id(mappings[i].mesh_id.toString());
					auto mesh = scene->getNodeByUniqueID(repo::core::model::RepoScene::GraphType::DEFAULT, id);
					if (mesh)
						mappingTree.addToTree(MP_LABEL_SHARED, mesh->getSharedID().toString());
					mappingTree.addToTree(MP_LABEL_MIN, mappings[i].min.toStdVector());
					mappingTree.addToTree(MP_LABEL_MAX, mappings[i].max.toStdVector());
					std::vector<std::string> usageArr = { meshUID + "_" + std::to_string(subMeshID) };
					mappingTree.addToTree(MP_LABEL_USAGE, usageArr);
					mappingTrees.push_back(mappingTree);
				}
			}
			else
			{
				repoError << "Failed to find split mapping for id: " << mappings[i].mesh_id;
			}
		}

		jsonTree.addArrayObjects(MP_LABEL_MAPPING, mappingTrees);

		std::string jsonFileName = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/" + mesh->getUniqueID().toString() + "_unity.json.mpc";

		jsonTrees[jsonFileName] = jsonTree;
	}
	else
	{
		repoError << "Unable to generate JSON file mapping : null pointer to mesh!";
	}

	return success;
}

bool AssetModelExport::generateTreeRepresentation()
{
	bool success;
	if (success = scene->hasRoot(gType))
	{
		auto meshes = scene->getAllMeshes(gType);

		std::vector<std::string> assetFiles, vrAssetFiles, iosAssetsFiles, androidAssetsFiles, jsons;
		for (const repo::core::model::RepoNode* node : meshes)
		{
			auto mesh = dynamic_cast<const repo::core::model::MeshNode*>(node);
			if (!mesh)
			{
				repoError << "Failed to cast a Repo Node of type mesh into a MeshNode (" << node->getUniqueID() << "). Skipping...";
				continue;
			}

			repo::manipulator::modelutility::MeshMapReorganiser* reSplitter;
			switch (mesh->getPrimitive())
			{
			case repo::core::model::MeshNode::Primitive::TRIANGLES:
				// For Triangles, the vertex limit is 65536, as Unity stores indices as shorts.
				// The maximum index array length in Unity is unknown, so the face limit is to SIZE_MAX.
				reSplitter = new repo::manipulator::modelutility::MeshMapReorganiser(mesh, ASSET_MAX_VERTEX_LIMIT, ASSET_MAX_TRIANGLE_LIMIT);
				break;
			case repo::core::model::MeshNode::Primitive::LINES:
				// For lines, each line (face) takes four vertices to draw (no-reuse between lines), so the face limit is 65536/4 to ensure they can all fit in the vertex buffer.
				reSplitter = new repo::manipulator::modelutility::MeshMapReorganiser(mesh, ASSET_MAX_VERTEX_LIMIT, ASSET_MAX_LINE_LIMIT);
				break;
			default:
				repoError << "MeshMapReorganiser cannot operate on node " << node->getUniqueID() << " because it has primitive type " << (int)mesh->getPrimitive() << " and AssetModelExport does not have known limits for this type. Skipping...";
				continue;
			}
				
			if (success = !std::make_shared<repo::core::model::MeshNode>(reSplitter->getRemappedMesh())->isEmpty())
			{
				reorganisedMeshes.push_back(std::make_shared<repo::core::model::MeshNode>(reSplitter->getRemappedMesh()));
				serialisedFaceBuf.push_back(reSplitter->getSerialisedFaces());
				idMapBuf.push_back(reSplitter->getIDMapArrays());
				meshMappings.push_back(reSplitter->getMappingsPerSubMesh());
				std::unordered_map<repo::lib::RepoUUID, std::vector<uint32_t>, repo::lib::RepoUUIDHasher> splitMapping = reSplitter->getSplitMapping();
				std::string fNamePrefix = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/" + mesh->getUniqueID().toString();
				if (generateVR) {
					vrAssetFiles.push_back(fNamePrefix + "_win64.unity3d");
					iosAssetsFiles.push_back(fNamePrefix + "_ios.unity3d");
					androidAssetsFiles.push_back(fNamePrefix + "_android.unity3d");
				}
				assetFiles.push_back(fNamePrefix + ".unity3d");
				jsons.push_back(fNamePrefix + "_unity.json.mpc");

				success &= generateJSONMapping(mesh, scene, reSplitter->getSplitMapping());
				delete reSplitter;
			}
			else
			{
				repoError << "Failed to generate a remapped mesh for mesh with ID : " << mesh->getUniqueID();
				break;
			}
		}

		std::string assetListFile = "/" + scene->getDatabaseName() + "/" + scene->getProjectName() + "/revision/" + scene->getRevisionID().toString() + "/unityAssets.json";
		repo::lib::PropertyTree assetListTree;
		assetListTree.addToTree(MP_LABEL_ASSETS, assetFiles);

		if (vrAssetFiles.size()) {
			assetListTree.addToTree(MP_LABEL_VR_ASSETS, vrAssetFiles);
		}

		if (iosAssetsFiles.size()) {
			assetListTree.addToTree(MP_LABEL_IOS_ASSETS, iosAssetsFiles);
		}

		assetListTree.addToTree(MP_LABEL_JSONS, jsons);
		assetListTree.addToTree(MP_LABEL_OFFSET, scene->getWorldOffset());
		assetListTree.addToTree(MP_LABEL_DATABASE, scene->getDatabaseName());
		assetListTree.addToTree(MP_LABEL_PROJECT, scene->getProjectName());
		jsonTrees[assetListFile] = assetListTree;

		unityAssets = core::model::RepoBSONFactory::makeRepoUnityAssets(
			scene->getRevisionID(),
			assetFiles,
			scene->getDatabaseName(),
			scene->getProjectName(),
			scene->getWorldOffset(),
			vrAssetFiles,
			iosAssetsFiles,
			androidAssetsFiles,
			jsons);
	}

	return success;
}