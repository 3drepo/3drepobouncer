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
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include "../../../core/model/bson/repo_bson_builder.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include "../../../lib/repo_log.h"

using namespace repo::manipulator::modelconvertor;
using namespace boost::property_tree;

RepoModelImport::RepoModelImport(const ModelImportConfig *settings) :
AbstractModelImport(settings)
{
	fin = nullptr;
	finCompressed = nullptr;
}

RepoModelImport::~RepoModelImport()
{
}

repo::core::model::MetadataNode* RepoModelImport::createMetadataNode(const ptree &metaTree, const std::string &parentName, const repo::lib::RepoUUID &parentID)
{
	std::vector<std::string> keys, values;

	for(ptree::const_iterator props = metaTree.begin(); props != metaTree.end(); props++)
	{
		std::string origKey = props->first;
		std::string key;
		char type = origKey[0];
		key = origKey.substr(1);

		keys.push_back(key);
		std::string value;
		switch (type)
		{
			case REPO_IMPORT_TYPE_BOOL:
				value = std::to_string(props->second.get_value<bool>());
				break;

			case REPO_IMPORT_TYPE_INT:
				value = std::to_string(props->second.get_value<int>());
				break;

			case REPO_IMPORT_TYPE_DOUBLE:
				value = std::to_string(props->second.get_value<double>());
				break;

			case REPO_IMPORT_TYPE_STRING:
				value = props->second.get_value<std::string>();
				break;
		}
		values.push_back(value);
	}


	repo::core::model::MetadataNode *metaNode = new repo::core::model::MetadataNode(
		repo::core::model::RepoBSONFactory::makeMetaDataNode(keys, values, parentName));

	metadata.insert(metaNode);

	return metaNode;
}

repo::core::model::MaterialNode *RepoModelImport::parseMaterial(const boost::property_tree::ptree &matTree)
{
		repo_material_t repo_material;

		if (matTree.find("diffuse") != matTree.not_found()) 
			repo_material.diffuse = as_vector<float>(matTree, "diffuse");
		else
			repo_material.diffuse.resize(3, 0.0f);

		if (matTree.find("specular") != matTree.not_found())
			repo_material.specular = as_vector<float>(matTree, "specular");
		else
			repo_material.specular.resize(3, 0.0f);

		if (matTree.find("emissive") != matTree.not_found())
			repo_material.emissive = as_vector<float>(matTree, "emissive");
		else
			repo_material.emissive.resize(3, 0.0f);

		if (matTree.find("ambient") != matTree.not_found())
			repo_material.ambient = as_vector<float>(matTree, "ambient");
		else
			repo_material.ambient.resize(3, 0.0f);

		if (matTree.find("transparency") != matTree.not_found())
			repo_material.opacity = 1.0f - matTree.get<float>("transparency");
		else
			repo_material.opacity = 1.0f;

		if (matTree.find("shininess") != matTree.not_found())
			repo_material.shininess = matTree.get<float>("shininess");
		else
			repo_material.shininess = 0.0f;

		repo_material.shininessStrength = 1.0f;
		repo_material.isWireframe = false;
		repo_material.isTwoSided = false;

		std::stringstream ss("");
		ss << "Material." << std::setfill('0') << std::setw(5) << materials.size();

		repo::core::model::MaterialNode * materialNode = new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(repo_material, ss.str(), REPO_NODE_API_LEVEL_1));
		materials.insert(materialNode);
		matNodeList.push_back(materialNode);

		return materialNode;
}

repo::core::model::MeshNode* RepoModelImport::createMeshNode(const ptree &mesh, const std::string &parentName, const repo::lib::RepoUUID &parentID, const repo::lib::RepoMatrix &trans)
{
	repo::core::model::MaterialNode *materialNode;

	int materialID = -1;

	int numIndices  = mesh.get<int>("numIndices");
	int numVertices = mesh.get<int>("numVertices");

	//Avoid using assimp objects everywhere -> converting assimp objects into repo structs
	std::vector<repo::lib::RepoVector3D> vertices;
	std::vector<repo::lib::RepoVector3D> normals;
	std::vector<repo_face_t> faces;

	std::vector<std::vector<float> > boundingBox;

	std::vector<std::vector<repo::lib::RepoVector2D>> uvChannels;
	std::vector<repo_color4d_t> colors;
	std::vector<std::vector<float> > outline;

	for(ptree::const_iterator props = mesh.begin(); props != mesh.end(); props++)
	{
		if (props->first == REPO_IMPORT_MATERIAL)
		{
			materialID = props->second.get_value<int>();
			materialNode = matNodeList[materialID];
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

	repo::lib::RepoVector3D min_test = vertices[0];
	repo::lib::RepoVector3D max_test = vertices[0];
	
	for(auto &v : vertices)
	{

		if (v.x < min_test.x) min_test.x = v.x;
		if (v.y < min_test.y) min_test.y = v.y;
		if (v.z < min_test.z) min_test.z = v.z;

		if (v.x > max_test.x) max_test.x = v.x;
		if (v.y > max_test.y) max_test.y = v.y;
		if (v.z > max_test.z) max_test.z = v.z;
		
		v.x -= offset[0];
		v.y -= offset[1];
		v.z -= offset[2];

		if (!trans.isIdentity())
		{
			v = trans * v;
		}

		if (v.x < min.x) min.x = v.x;
		if (v.y < min.y) min.y = v.y;
		if (v.z < min.z) min.z = v.z;

		if (v.x > max.x) max.x = v.x;
		if (v.y > max.y) max.y = v.y;
		if (v.z > max.z) max.z = v.z;
	}

	if (!trans.isIdentity())
	{
		repo::lib::RepoMatrix invTrans;

		invTrans = trans.invert().transpose();

		auto data = invTrans.getData();
		data[3] = data[7] = data[11] = 0;
		data[12] = data[13] = data[14] = 0;

		repo::lib::RepoMatrix normTrans(data);

		for(auto &n : normals)
		{
			n = normTrans * n;
		}
	}

	std::vector<float> minBBox;
	std::vector<float> maxBBox;

	minBBox.push_back(min.x); minBBox.push_back(min.y); minBBox.push_back(min.z);
	maxBBox.push_back(max.x); maxBBox.push_back(max.y); maxBBox.push_back(max.z);

	boundingBox.push_back(minBBox);
	boundingBox.push_back(maxBBox);

	repo::core::model::MeshNode *meshNode = new repo::core::model::MeshNode(repo::core::model::RepoBSONFactory::makeMeshNode(
		vertices, faces, normals, boundingBox, uvChannels, colors, outline));

	*meshNode = meshNode->cloneAndAddParent(parentID);

	if (materialID >= 0)
	{
		matParents[materialID].push_back(meshNode->getSharedID());
	}

	meshes.insert(meshNode);

	return meshNode;
}

void RepoModelImport::createObject(const ptree& tree)
{
	int myID     = tree.get<int>("id");
	int myParent = tree.get<int>("parent");

	std::string transName = tree.get<std::string>("name", "");

	if (myParent > node_map.size())
	{
		repoError << "Invalid parent ID: " << myParent;
	}

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

void RepoModelImport::skipAheadInFile(long amount)
{
	if (amount > 0)
	{
		// Cannot use seekg on GZIP file
		char *tmpBuf = new char[amount];
		fin->read(tmpBuf, amount);
		delete[] tmpBuf;
	}
}

bool RepoModelImport::importModel(std::string filePath, std::string &errMsg)
{
	orgFile = filePath;
	std::string fileName = getFileName(filePath);

	repoInfo << "IMPORT [" << fileName << "]";
	repoInfo << "=== IMPORTING MODEL WITH REPO IMPORTER ===";

	finCompressed = new std::ifstream(filePath, std::ios_base::in | std::ios::binary);

	if(finCompressed)
	{
		inbuf = new boost::iostreams::filtering_streambuf<boost::iostreams::input>();

#ifndef REPO_BOOST_NO_GZIP
		inbuf->push(boost::iostreams::gzip_decompressor());
#else
		repoWarning << "Gzip is not compiled into Boost library, .bim imports may not work as intended";
#endif
		inbuf->push(*finCompressed);

		fin = new std::istream(inbuf);

		const int fileVersionSize = strlen(supportedFileVersion);
		char *fileVersion = (char*) malloc(sizeof(*fileVersion *  fileVersionSize));

		fin->read(fileVersion, fileVersionSize);

		if (strcmp(fileVersion, supportedFileVersion) != 0)
		{
			repoError << "Unsupported BIM file version" << fileVersion;
			return false;
		}

		repoInfo << "Loading BIM file [VERSION: " << fileVersion << "]";

		delete fileVersion;
		
		size_t metaSize = fileVersionSize + sizeof(fileMeta);
		// Size of metadata at start
		fin->read((char*)&file_meta, sizeof(fileMeta));

		repoInfo << "META size: " << metaSize;
		repoInfo << "SIZE: header = " << file_meta.headerSize << " bytes, geometry = " << file_meta.geometrySize << " bytes.";
		repoInfo << "SIZE ARRAY: location = " << file_meta.sizesStart << " bytes, size = " << file_meta.sizesSize << " bytes.";
		repoInfo << "MAT ARRAY: location = " << file_meta.matStart << " bytes, size = " << file_meta.matSize << " bytes.";
		repoInfo << "Number of parts to process : " << file_meta.numChildren;

		
		if (file_meta.matStart > file_meta.sizesStart)
		{
			skipAheadInFile(file_meta.sizesStart - metaSize);
			sizes = as_vector<long>(getNextJSON(file_meta.sizesSize));

			skipAheadInFile(file_meta.matStart - (file_meta.sizesStart + file_meta.sizesSize));

			boost::property_tree::ptree materials = getNextJSON(file_meta.matSize);
			for (const auto& item : materials) {
				parseMaterial(item.second);
			}
			matParents.resize(materials.size());

			skipAheadInFile(file_meta.headerSize + metaSize - (file_meta.matStart + file_meta.matSize));
		} else {
			skipAheadInFile(file_meta.matStart - metaSize);

			boost::property_tree::ptree materialsArr = getNextJSON(file_meta.matSize);
			for (const auto& item : materialsArr) {
				parseMaterial(item.second);
			}

			matParents.resize(materials.size());

			skipAheadInFile(file_meta.sizesStart - (file_meta.matStart + file_meta.matSize));
			sizes = as_vector<long>(getNextJSON(file_meta.sizesSize));

			skipAheadInFile(file_meta.headerSize + metaSize - (file_meta.sizesStart + file_meta.sizesSize));			
		}

		repoInfo << "Reading geometry buffer";

		geomBuf = new char[file_meta.geometrySize];
		fin->read(geomBuf, file_meta.geometrySize);

		finCompressed->close();
		delete finCompressed;

		finCompressed = new std::ifstream(filePath, std::ios_base::in | std::ios::binary);

		delete fin;
		delete inbuf;

		inbuf = new boost::iostreams::filtering_streambuf<boost::iostreams::input>();
#ifndef REPO_BOOST_NO_GZIP
		inbuf->push(boost::iostreams::gzip_decompressor());
#endif
		inbuf->push(*finCompressed);

		fin = new std::istream(inbuf);

		// Skip to the root start. No seekg for GZIPped files.
		skipAheadInFile(sizes[0]);

		return true;
	} else {
		repoError << "File " << fileName << " not found.";
		return false;
	}
}

boost::property_tree::ptree RepoModelImport::getNextJSON(long jsonSize)
{
	boost::property_tree::ptree singleJSON;
	char *jsonBuf = new char[jsonSize + 1];

	fin->read(jsonBuf, jsonSize);
	jsonBuf[jsonSize] = '\0'; 

	std::stringstream bufReader(jsonBuf);
	read_json(bufReader, singleJSON);

	delete[] jsonBuf;

	return singleJSON;
}

repo::core::model::RepoScene* RepoModelImport::generateRepoScene()
{
	repoInfo << "Generating scene";

	// Process root node
	boost::property_tree::ptree root = getNextJSON(sizes[1]);
	std::string rootName = root.get<std::string>("name", "");

	boost::optional< ptree& > rootBBOX = root.get_child_optional("bbox");

	if (!rootBBOX) {
		repoError << "No root bounding box specified.";
		return nullptr;
	}

	offset = as_vector<double>(*rootBBOX, "min");

	boost::optional< ptree& > transMatTree = root.get_child_optional("transformation");
	repo::lib::RepoMatrix transMat;

	if (transMatTree)
	{
		transMat = repo::lib::RepoMatrix(as_vector<float>(root, "transformation"));
	}

	repo::core::model::TransformationNode *rootNode =
		new repo::core::model::TransformationNode(
		repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), rootName, std::vector<repo::lib::RepoUUID>()));

	node_map.push_back(rootNode);
	trans_map.push_back(transMat);
	transformations.insert(rootNode);

	char comma;

	for(long i = 0; i < file_meta.numChildren; i++)
	{
		if (i % 500 == 0 || i == file_meta.numChildren - 1)
		{
			repoInfo << "Importing " << i << " of " << file_meta.numChildren << " JSON nodes";
		}

		fin->read(&comma, 1);
		boost::property_tree::ptree jsonTree = getNextJSON(sizes[i + 2]);
		createObject(jsonTree);
	}

	materials.clear();

	repoInfo << "Attaching materials to parents";
	// Attach all the parents to the materials
	for (int i = 0; i < matNodeList.size(); i++)
	{
		repo::core::model::MaterialNode *tmpMaterial = new repo::core::model::MaterialNode();
		*tmpMaterial = matNodeList[i]->cloneAndAddParent(matParents[i]);
		materials.insert(tmpMaterial);
	}

	std::vector<std::string> fileVect;
	if (!orgFile.empty())
		fileVect.push_back(orgFile);

	repo::core::model::RepoScene * scenePtr = new repo::core::model::RepoScene(fileVect, cameras, meshes, materials, metadata, textures, transformations);

	// Transform offset into correct space
	repo::lib::RepoVector3D tmpVec;

	tmpVec.x = offset[0];
	tmpVec.y = offset[1];
	tmpVec.z = offset[2];

	tmpVec = transMat * tmpVec;

	offset[0] = tmpVec.x;
	offset[1] = tmpVec.y;
	offset[2] = tmpVec.z;

	scenePtr->setWorldOffset(offset);

	delete[] geomBuf;

	finCompressed->close();

	delete fin;
	delete inbuf;
	delete finCompressed;

	return scenePtr;
}
