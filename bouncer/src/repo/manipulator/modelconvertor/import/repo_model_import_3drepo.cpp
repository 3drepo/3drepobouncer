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
* Allows Import/Export functionality into/output Repo world using IFCOpenShell
*/

#include "repo_model_import_3drepo.h"
#include <boost/filesystem.hpp>
#include <sstream>
#include <fstream>
#include <iostream>
#include "../../../core/model/bson/repo_bson_builder.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../lib/repo_log.h"

using namespace repo::manipulator::modelconvertor;
using namespace boost::property_tree;

RepoModelImport::RepoModelImport(const ModelImportConfig *settings) :
AbstractModelImport(settings)
{
}

RepoModelImport::~RepoModelImport()
{
}

repo::core::model::MetadataNode* RepoModelImport::createMetadataNode(const ptree& metaTree, std::string parentName, repo::lib::RepoUUID &parentID)
{
	//build the metadata as a bson
	repo::core::model::RepoBSONBuilder builder;

	for(ptree::const_iterator props = metaTree.begin(); props != metaTree.end(); props++)
	{
		std::string origKey = props->first;
		std::string key;
		char type = origKey[0];
		key = origKey.substr(1);

		switch (type)
		{
			case REPO_IMPORT_TYPE_BOOL:
				builder << key << props->second.get_value<bool>();
				break;

			case REPO_IMPORT_TYPE_INT:
				builder << key << props->second.get_value<int>();
				break;

			case REPO_IMPORT_TYPE_DOUBLE:
				builder << key << props->second.get_value<double>();
				break;

			case REPO_IMPORT_TYPE_STRING:
				builder << key << props->second.get_value<std::string>();
				break;
		}
	}

	repo::core::model::RepoBSON metaBSON = builder.obj();
	//std::vector<repo::lib::RepoUUID> parentIDs;
	//parentIDs.push_back(parentID);

	repo::core::model::MetadataNode *metaNode = new repo::core::model::MetadataNode(
		repo::core::model::RepoBSONFactory::makeMetaDataNode(metaBSON, "", parentName));

	metadata.insert(metaNode);

	return metaNode;
}

//http://stackoverflow.com/questions/23481262/using-boost-property-tree-to-read-int-array
template <typename T>
std::vector<T> as_vector(ptree const& pt, ptree::key_type const& key)
{
    std::vector<T> r;
    for (auto& item : pt.get_child(key))
        r.push_back(item.second.get_value<T>());
    return r;
}

repo::core::model::MeshNode* RepoModelImport::createMeshNode(const ptree& mesh, std::string parentName, repo::lib::RepoUUID &parentID, const repo::lib::RepoMatrix& trans)
{
	repo::core::model::MaterialNode *materialNode;

	bool hasMaterial = false;

	int numIndices  = mesh.get<int>("numIndices");
	int numVertices = mesh.get<int>("numVertices");

	/*
	std::vector<int> faces;
	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> colors;
	std::vector<std::vector<float>> uvChannels;
	std::vector<std::vector<float>> outline;
	*/

	//Avoid using assimp objects everywhere -> converting assimp objects into repo structs
	std::vector<repo::lib::RepoVector3D> vertices;
	std::vector<repo::lib::RepoVector3D> normals;
	std::vector<repo_face_t> faces;

	std::vector<std::vector<float> > boundingBox;

	std::vector<std::vector<repo::lib::RepoVector2D>> uvChannels;
	std::vector<repo_color4d_t> colors;
	std::vector<std::vector<float> > outline;

	//faces.resize(numIndices);
	//vertices.resize(numVertices); normals.resize(numVertices); 

	for(ptree::const_iterator props = mesh.begin(); props != mesh.end(); props++)
	{
		if (props->first == REPO_IMPORT_MATERIAL)
		{
			repo_material_t repo_material;
			
			if (props->second.find("diffuse") != props->second.not_found())
			{
				repo_material.diffuse = as_vector<float>(props->second, "diffuse");
			}

			if (props->second.find("transparency") != props->second.not_found())
			{
				repo_material.opacity = 1.0f - props->second.get<float>("transparency");
			} else {
				repo_material.opacity = 1.0f;
			}

			hasMaterial = true;

			materialNode = new repo::core::model::MaterialNode(
			repo::core::model::RepoBSONFactory::makeMaterialNode(repo_material, parentName, REPO_NODE_API_LEVEL_1));
		}


		if (props->first == REPO_IMPORT_VERTICES || props->first == REPO_IMPORT_NORMALS)
		{
			std::vector<int> startEnd = as_vector<int>(mesh, props->first);

			float *tmpVertices = (float *)(geomBuf + startEnd[0]);

			for(int i = 0; i < numVertices; i ++)
			{
				repo::lib::RepoVector3D tmpVec;

				tmpVec.x = tmpVertices[i * 3];
				tmpVec.y = tmpVertices[i * 3 + 1];
				tmpVec.z = tmpVertices[i * 3 + 2];

				//if (props->first == REPO_IMPORT_VERTICES)
				//	repoInfo << "NV: " << numVertices << " V: " << tmpVec.x << " " << tmpVec.y << " " << tmpVec.z;

				if (props->first == REPO_IMPORT_VERTICES )
					vertices.push_back(tmpVec);
				else
					normals.push_back(tmpVec);
			}
		}

		if (props->first == REPO_IMPORT_INDICES)
		{
			std::vector<int> startEnd = as_vector<int>(mesh, REPO_IMPORT_INDICES);

			uint32_t *tmpIndices = (uint32_t*)(geomBuf + startEnd[0]);

			for(int i = 0; i < numIndices; i += 3)
			{
				repo_face_t tmpFace;

				tmpFace.push_back(tmpIndices[i]);
				tmpFace.push_back(tmpIndices[i + 1]);
				tmpFace.push_back(tmpIndices[i + 2]);

				faces.push_back(tmpFace);
			}
		}
	}

	repo::lib::RepoVector3D min = vertices[0];
	repo::lib::RepoVector3D max = vertices[0];


	for(auto &v : vertices)
	{
		if (v.x < min.x) min.x = v.x;
		if (v.y < min.y) min.y = v.y;
		if (v.z < min.z) min.z = v.z;

		if (v.x > max.x) max.x = v.x;
		if (v.y > max.y) max.y = v.y;
		if (v.z > max.z) max.z = v.z;

		if (!trans.isIdentity())
		{
			v = trans * v;
		}

		v.x -= offset[0];
		v.y -= offset[1];
		v.z -= offset[2];
	}

	min = trans * min;
	max = trans * max;

	/*
	if (sceneMin.size() == 0)
	{
		for(int i = 0; i < 3; i++)
		{
			sceneMin.push_back(min[i]);
			sceneMax.push_back(max[i]);
		}
	} 
	else {
		min.x = (sceneMin[0] < min.x) ? sceneMin[0] : min.x;
		min.y = (sceneMin[1] < min.y) ? sceneMin[1] : min.y;
		min.z = (sceneMin[2] < min.z) ? sceneMin[2] : min.z;

		max.x = (sceneMax[0] > max.x) ? sceneMax[0] : max.x;
		max.y = (sceneMax[1] > max.y) ? sceneMax[1] : max.y;
		max.z = (sceneMax[2] > max.z) ? sceneMax[2] : max.z;
	}*/

		
	std::vector<float> minBBox; // = as_vector<float>(props->second, "min");
	std::vector<float> maxBBox; // = as_vector<float>(props->second, "max");
	
	//repoInfo << "BBOX: " << min.x << " " << min.y << " " << min.z;
	//repoInfo << "BBOX: " << max.x << " " << max.y << " " << max.z;

	minBBox.push_back(min.x); minBBox.push_back(min.y); minBBox.push_back(min.z);
	maxBBox.push_back(max.x); maxBBox.push_back(max.y); maxBBox.push_back(max.z);

	boundingBox.push_back(minBBox);
	boundingBox.push_back(maxBBox);
	
	repo::core::model::MeshNode *meshNode = new repo::core::model::MeshNode(repo::core::model::RepoBSONFactory::makeMeshNode(
		vertices, faces, normals, boundingBox, uvChannels, colors, outline));

	*meshNode = meshNode->cloneAndAddParent(parentID);

	if (hasMaterial)
	{
		*materialNode = materialNode->cloneAndAddParent(meshNode->getSharedID());
		materials.insert(materialNode);
	}

	meshes.insert(meshNode);

	return meshNode;
}

void RepoModelImport::createObject(const ptree& tree)
{
	int myID     = tree.get<int>("id");
	int myParent = tree.get<int>("parent");

	std::string transName = tree.get<std::string>("name", "");

	repo::lib::RepoUUID parentSharedID = node_map[myParent]->getSharedID();
	repo::lib::RepoMatrix parentTransform = trans_map[myParent];

	std::vector<repo::lib::RepoUUID> parentIDs;
	parentIDs.push_back(parentSharedID);

	boost::optional< const ptree& > transMatTree = tree.get_child_optional("transformation");
	
	repo::lib::RepoMatrix transMat;

	if (transMatTree)
	{
		transMat = repo::lib::RepoMatrix(as_vector<float>(tree, "transformation"));
	}

	trans_map.push_back(parentTransform * transMat);

	repo::core::model::TransformationNode * transNode =
		new repo::core::model::TransformationNode(
		repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), transName, parentIDs));

	repo::lib::RepoUUID transID = transNode->getSharedID();

	//repoInfo << "TRANS [" << node_map[myParent]->getName() << "][" << transName << "] : " << std::endl << trans_map.back().toString();

	node_map.push_back(transNode);

	std::vector<repo::core::model::MetadataNode*> metas;
	std::vector<repo::core::model::MeshNode*> meshes;

	for(ptree::const_iterator props = tree.begin(); props != tree.end(); props++)
	{
		if (props->first == REPO_IMPORT_METADATA)
		{
			metas.push_back(createMetadataNode(props->second, transName, parentSharedID));
		}

		if (props->first == REPO_IMPORT_GEOMETRY)
		{
			meshes.push_back(createMeshNode(props->second, transName, transID, trans_map.back()));
		}
	}

	repo::lib::RepoUUID metaParentSharedID = transNode->getSharedID();

	if (meshes.size() > 0)
	{
		if (meshes.size() > 1)
			repoWarning << "More than one mesh to attach metadata to.";

		metaParentSharedID = meshes[0]->getSharedID();
	}

	for(auto &meta : metas)
	{
		*meta = meta->cloneAndAddParent(metaParentSharedID);
	}

	transformations.insert(transNode);
}

bool RepoModelImport::importModel(std::string filePath, std::string &errMsg)
{
	std::string fileName = getFileName(filePath);

	repoInfo << "IMPORT [" << fileName << "]";
	repoInfo << "=== IMPORTING MODEL WITH REPO IMPORTER ===";
	
	std::ifstream fin(filePath, std::ios::binary);

	int64_t headerSize, geometrySize;

	fin.read((char*)&headerSize, sizeof(int64_t));
	fin.read((char*)&geometrySize, sizeof(int64_t));
	
	repoInfo << "HEADER: " << headerSize << " GEOMETRY: " << geometrySize;
	
	char *jsonBuf = new char[headerSize];
	fin.read(jsonBuf, headerSize);

	//std::cout << jsonBuf << std::endl;

	geomBuf = new char[geometrySize];
	fin.read(geomBuf, geometrySize);

	std::stringstream bufReader(jsonBuf);
	read_json(bufReader, jsonHeader);

	fin.close();
	delete[] jsonBuf;

	return true;
}

repo::core::model::RepoScene* RepoModelImport::generateRepoScene()
{
	ptree::assoc_iterator children = jsonHeader.find("parts");
	
	repo::lib::RepoMatrix mat;

	
	for(ptree::iterator child = children->second.begin(); child != children->second.end(); child++)
	{
		if (child->second.find("parent") == child->second.not_found())
		{
			std::string rootName = child->second.get<std::string>("name", "");

			boost::optional< ptree& > rootBBOX = child->second.get_child_optional("bbox");

			if (!rootBBOX) {
				repoError << "No root bounding box specified.";
				return nullptr;
			}

			offset = as_vector<double>(*rootBBOX, "min");

			repoInfo << "ROOT: " << rootName;
				
			for(int i = 0; i < 3; i++)
			{
				repoInfo << "SM: " << offset[i];
			}

			boost::optional< ptree& > transMatTree = child->second.get_child_optional("transformation");
			repo::lib::RepoMatrix transMat;

			if (transMatTree)
			{
				transMat = repo::lib::RepoMatrix(as_vector<float>(child->second, "transformation"));
			}

			repo::core::model::TransformationNode *rootNode =
				new repo::core::model::TransformationNode(
				repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), rootName, std::vector<repo::lib::RepoUUID>()));

			node_map.push_back(rootNode);
			trans_map.push_back(transMat);
			transformations.insert(rootNode);
		} else {
			createObject(child->second);
		}
	}

	std::vector<std::string> fileVect;
	if (!orgFile.empty())
		fileVect.push_back(orgFile);

	repo::core::model::RepoScene * scenePtr = new repo::core::model::RepoScene(fileVect, cameras, meshes, materials, metadata, textures, transformations);

	scenePtr->setWorldOffset(offset);

	delete[] geomBuf;

	return scenePtr;
}