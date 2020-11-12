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
#include "../../../error_codes.h"

using namespace repo::manipulator::modelconvertor;
using namespace boost::property_tree;

RepoModelImport::RepoModelImport(const ModelImportConfig &settings) :
	AbstractModelImport(settings)
{
	fin = nullptr;
	finCompressed = nullptr;
}

RepoModelImport::~RepoModelImport()
{
}

repo::core::model::MetadataNode* RepoModelImport::createMetadataNode(
	const ptree &metaTree, 
	const std::string &parentName, 
	const repo::lib::RepoUUID &parentID)
{
	std::vector<std::string> keys, values;

	for (ptree::const_iterator props = metaTree.begin(); props != metaTree.end(); props++)
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

repo::core::model::MaterialNode* RepoModelImport::parseMaterial(const boost::property_tree::ptree &matTree)
{
	repo_material_t repo_material;
	int textureId = -1;

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
		repo_material.shininess = matTree.get<float>("shininess") * 5;
	else
		repo_material.shininess = 0.0f;

	if (matTree.find("texture") != matTree.not_found())
	{
		textureId = matTree.get<int>("texture");
	}

	repo_material.shininessStrength = 0.25f;
	repo_material.isWireframe = false;
	repo_material.isTwoSided = false;

	std::stringstream ss("");
	ss << "Material." << std::setfill('0') << std::setw(5) << materials.size();

	repo::core::model::MaterialNode * materialNode = new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(repo_material, ss.str(), REPO_NODE_API_LEVEL_1));
	materials.insert(materialNode);
	matNodeList.push_back(materialNode);

	if(textureId >= 0)
	{
		textureIdToParents[textureId].push_back(materialNode->getSharedID());
	}

	return materialNode;
}

repo::core::model::TextureNode* RepoModelImport::parseTexture(const boost::property_tree::ptree& textureTree, char * dataBuffer)
{
	std::string name = textureTree.get_child("filename").data();
	uint32_t byteCount = textureTree.get<uint32_t>("numImageBytes");
	uint32_t width = textureTree.get<uint32_t>("width");
	uint32_t height = textureTree.get<uint32_t>("height");
	uint32_t id = textureTree.get<uint32_t>("id");

	std::vector<uint32_t> DataStartEnd = as_vector<uint32_t>(textureTree, "imageBytes");
	// Error if size is not 2

	char* data = &dataBuffer[DataStartEnd[0]];

	repo::core::model::TextureNode* textureNode =
		new repo::core::model::TextureNode(
			repo::core::model::RepoBSONFactory::makeTextureNode(
				name,
				data,
				byteCount,
				width,
				height,
				REPO_NODE_API_LEVEL_1));
	
	repo::core::model::TextureNode* tmpTexture = new repo::core::model::TextureNode();
	*tmpTexture = textureNode->cloneAndAddParent(textureIdToParents[id]);
	textures.insert(tmpTexture);

	return textureNode;
}


RepoModelImport::mesh_data_t RepoModelImport::createMeshRecord(
	const ptree &mesh,
	const std::string &parentName,
	const repo::lib::RepoUUID &parentID,
	const repo::lib::RepoMatrix &trans)
{
	int materialID = -1;

	int numIndices = mesh.get<int64_t>("numIndices");
	int numVertices = mesh.get<int64_t>("numVertices");

	std::vector<repo::lib::RepoVector3D64> vertices;
	std::vector<repo::lib::RepoVector3D> normals;
	std::vector<repo_face_t> faces;

	std::vector<std::vector<double> > boundingBox;

	const bool needTransform = !trans.isIdentity();
	repo::lib::RepoMatrix normalTrans;

	if (needTransform)
	{
		repo::lib::RepoMatrix invTrans;

		invTrans = trans.invert().transpose();

		auto data = invTrans.getData();
		data[3] = data[7] = data[11] = 0;
		data[12] = data[13] = data[14] = 0;

		normalTrans = repo::lib::RepoMatrix(data);
	}

	std::vector<double> minBBox;
	std::vector<double> maxBBox;

	for (ptree::const_iterator props = mesh.begin(); props != mesh.end(); props++)
	{
		if (props->first == REPO_IMPORT_MATERIAL)
		{
			materialID = props->second.get_value<int>();
		}

		if (props->first == REPO_IMPORT_VERTICES || props->first == REPO_IMPORT_NORMALS)
		{
			std::vector<int64_t> startEnd = as_vector<int64_t>(mesh, props->first);

			double *tmpVerticesDouble = (double *)(dataBuffer + startEnd[0]);
			float *tmpVerticesSingle = (float *)(dataBuffer + startEnd[0]);

			for (int i = 0; i < numVertices; i++)
			{
				if (props->first == REPO_IMPORT_VERTICES) {
					repo::lib::RepoVector3D64 tmpVec;
					if (areVertices32Bit)
						tmpVec = { tmpVerticesSingle[i * 3], tmpVerticesSingle[i * 3 + 1], tmpVerticesSingle[i * 3 + 2] };
					else
						tmpVec = { tmpVerticesDouble[i * 3] ,  tmpVerticesDouble[i * 3 + 1] , tmpVerticesDouble[i * 3 + 2] };
					if (needTransform) tmpVec = trans * tmpVec;

					if (minBBox.size()) {
						if (tmpVec.x < minBBox[0]) minBBox[0] = tmpVec.x;
						if (tmpVec.y < minBBox[1]) minBBox[1] = tmpVec.y;
						if (tmpVec.z < minBBox[2]) minBBox[2] = tmpVec.z;
					}
					else
						minBBox = { tmpVec.x, tmpVec.y, tmpVec.z };

					if (maxBBox.size()) {
						if (tmpVec.x > maxBBox[0]) maxBBox[0] = tmpVec.x;
						if (tmpVec.y > maxBBox[1]) maxBBox[1] = tmpVec.y;
						if (tmpVec.z > maxBBox[2]) maxBBox[2] = tmpVec.z;
					}
					else
						maxBBox = { tmpVec.x, tmpVec.y, tmpVec.z };

					vertices.push_back(tmpVec);
				}
				else {
					repo::lib::RepoVector3D tmpVec = { tmpVerticesSingle[i * 3] ,  tmpVerticesSingle[i * 3 + 1] , tmpVerticesSingle[i * 3 + 2] };
					normals.push_back(needTransform ? normalTrans * tmpVec : tmpVec);
				}
			}
		}

		if (props->first == REPO_IMPORT_INDICES)
		{
			std::vector<int64_t> startEnd = as_vector<int64_t>(mesh, REPO_IMPORT_INDICES);

			uint32_t *tmpIndices = (uint32_t*)(dataBuffer + startEnd[0]);

			for (int i = 0; i < numIndices; i += 3)
			{
				repo_face_t tmpFace;

				tmpFace.push_back(tmpIndices[i]);
				tmpFace.push_back(tmpIndices[i + 1]);
				tmpFace.push_back(tmpIndices[i + 2]);

				faces.push_back(tmpFace);
			}
		}
	}

	boundingBox.push_back(minBBox);
	boundingBox.push_back(maxBBox);

	if (offset.size()) {
		for (int i = 0; i < offset.size(); ++i)
			offset[i] = offset[i] > minBBox[i] ? minBBox[i] : offset[i];
	}
	else {
		offset = minBBox;
	}

	const auto sharedID = repo::lib::RepoUUID::createUUID();

	if (materialID >= 0)
	{
		matParents[materialID].push_back(sharedID);
	}

	mesh_data_t result = { vertices, normals, faces, boundingBox, parentID, sharedID };
	return result;
}

void RepoModelImport::createObject(const ptree& tree)
{
	int myID = tree.get<int>("id");
	int myParent = tree.get<int>("parent");

	std::string transName = tree.get<std::string>("name", "");

	if (myParent > node_map.size())
	{
		repoError << "Invalid parent ID: " << myParent;
	}

	repo::lib::RepoUUID parentSharedID = node_map[myParent]->getSharedID();
	repo::lib::RepoMatrix parentTransform = trans_matrix_map[myParent];

	std::vector<repo::lib::RepoUUID> parentIDs;
	parentIDs.push_back(parentSharedID);

	boost::optional< const ptree& > transMatTree = tree.get_child_optional("transformation");

	repo::lib::RepoMatrix transMat;

	if (transMatTree)
	{
		transMat = repo::lib::RepoMatrix(as_vector<float>(tree, "transformation"));
	}

	trans_matrix_map.push_back(parentTransform * transMat);

	repo::core::model::TransformationNode * transNode =
		new repo::core::model::TransformationNode(
			repo::core::model::RepoBSONFactory::makeTransformationNode(repo::lib::RepoMatrix(), transName, parentIDs));

	repo::lib::RepoUUID transID = transNode->getSharedID();

	node_map.push_back(transNode);

	std::vector<repo::core::model::MetadataNode*> metas;
	std::vector<repo::lib::RepoUUID> metaParentIDs;

	for (ptree::const_iterator props = tree.begin(); props != tree.end(); props++)
	{
		if (props->first == REPO_IMPORT_METADATA)
		{
			metas.push_back(createMetadataNode(props->second, transName, parentSharedID));
		}

		if (props->first == REPO_IMPORT_GEOMETRY)
		{
			auto mesh = createMeshRecord(props->second, transName, transID, trans_matrix_map.back());
			metaParentIDs.push_back(mesh.sharedID);
			meshEntries.push_back(mesh);
		}
	}

	metaParentIDs.push_back(transNode->getSharedID());

	for (auto &meta : metas)
	{
		*meta = meta->cloneAndAddParent(metaParentIDs);
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

bool RepoModelImport::importModel(std::string filePath, uint8_t &err)
{
	orgFile = filePath;
	std::string fileName = getFileName(filePath);

	repoInfo << "IMPORT [" << fileName << "]";
	repoInfo << "=== IMPORTING MODEL WITH REPO IMPORTER ===";

	// Reading in file
	finCompressed = new std::ifstream(filePath, std::ios_base::in | std::ios::binary);
	if (finCompressed)
	{
		inbuf = new boost::iostreams::filtering_streambuf<boost::iostreams::input>();
#ifndef REPO_BOOST_NO_GZIP
		inbuf->push(boost::iostreams::gzip_decompressor());
#else
		repoWarning << "Gzip is not compiled into Boost library, .bim imports may not work as intended";
#endif
		inbuf->push(*finCompressed);
		fin = new std::istream(inbuf);

		// Check the BIM file format version
		char fileVersion[REPO_VERSION_LENGTH + 1] = { 0 };
		fin->read(fileVersion, REPO_VERSION_LENGTH);
		std::string incomingVersion = fileVersion;
		if (supportedFileVersions.find(incomingVersion) == supportedFileVersions.end())
		{
			repoError << "Unsupported BIM file version: " << fileVersion;
			err = REPOERR_UNSUPPORTED_BIM_VERSION;
			return false;
		}
		areVertices32Bit = REPO_V1 == incomingVersion;

		// Loading file metadata
		repoInfo << "Loading BIM file [VERSION: " << incomingVersion << "] 32 bit? : " << areVertices32Bit;
		size_t metaSize = REPO_VERSION_LENGTH + sizeof(fileMeta);
		if(incomingVersion == REPO_V3)
		{
			fin->read((char*)&file_meta, REPO_V3_FILEMETA_BYTE_LEN);
		}
		else
		{
			fin->read((char*)&file_meta, REPO_V2_FILEMETA_BYTE_LEN);
		}
		repoInfo << std::left << std::setw(30) << "File meta size: "				<< metaSize;
		repoInfo << std::left << std::setw(30) << "JSON size: "						<< file_meta.jsonSize	  << " bytes";
		repoInfo << std::left << std::setw(30) << "Data buffer size: "				<< file_meta.dataSize	  << " bytes";
		repoInfo << std::left << std::setw(30) << "\"sizes\" array start location: "<< file_meta.sizesStart	  << " bytes";
		repoInfo << std::left << std::setw(30) << "\"sizes\" array size: "			<< file_meta.sizesSize	  << " bytes";
		repoInfo << std::left << std::setw(30) << "\"materials\" array location: "	<< file_meta.matStart	  << " bytes";
		repoInfo << std::left << std::setw(30) << "\"materials\" array size: "		<< file_meta.matSize	  << " bytes";
		repoInfo << std::left << std::setw(30) << "\"textures\" array location: "	<< file_meta.textureSize  << " bytes";
		repoInfo << std::left << std::setw(30) << "\"textures\" array size: "		<< file_meta.textureStart << " bytes";
		repoInfo << std::left << std::setw(30) << "Number of parts to process:"		<< file_meta.numChildren;

		// Load full JSON tree
		boost::property_tree::ptree jsonRoot = getNextJSON(file_meta.jsonSize);

		// Load binary data
		repoInfo << "Reading data buffer";
		dataBuffer = new char[file_meta.dataSize];
		fin->read(dataBuffer, file_meta.dataSize);

		// Loading in required JSON nodes
		boost::optional<ptree&> materialsRoot = jsonRoot.get_child_optional("materials");
		if (materialsRoot)
		{
			for (ptree::value_type element : materialsRoot.get())
			{
				parseMaterial(element.second);
			}
			matParents.resize(materials.size());
			repoInfo << "Loaded: " << materials.size() << " materials";
		}
		else
		{
			repoError << "File " << fileName << " does not have a \"materials\" node";
			err = REPOERR_MODEL_FILE_READ;
			return false;
		}
		boost::optional<ptree&> sizesRoot = jsonRoot.get_child_optional("sizes");
		if(sizesRoot)
		{
			sizes = as_vector<long>(sizesRoot.get());
		}
		else
		{
			repoError << "File " << fileName << " does not have a \"sizes\" node";
			err = REPOERR_MODEL_FILE_READ;
			return false;
		}
		boost::optional<ptree&> texturesRoot = jsonRoot.get_child_optional("textures");
		if (texturesRoot)
		{
			for (ptree::value_type element : texturesRoot.get())
			{
				parseTexture(element.second, dataBuffer);
			}
			repoInfo << "Loaded: " << textures.size() << " textures";
		}

		// Clean up
		finCompressed->close();
		delete finCompressed;
		finCompressed = new std::ifstream(filePath, std::ios_base::in | std::ios::binary);
		delete fin;
		delete inbuf;

		// Reloading file, skipping to the root start. No seekg for GZIPped files.
		inbuf = new boost::iostreams::filtering_streambuf<boost::iostreams::input>();
#ifndef REPO_BOOST_NO_GZIP
		inbuf->push(boost::iostreams::gzip_decompressor());
#endif
		inbuf->push(*finCompressed);
		fin = new std::istream(inbuf);
		skipAheadInFile(sizes[0]);

		return true;
	}
	else {
		repoError << "File " << fileName << " not found.";
		err = REPOERR_MODEL_FILE_READ;
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

repo::core::model::RepoScene* RepoModelImport::generateRepoScene(uint8_t &errMsg)
{
	repoInfo << "Generating scene";

	// Process root node
	boost::property_tree::ptree root = getNextJSON(sizes[1]);
	std::string rootName = root.get<std::string>("name", "");
	boost::optional< ptree& > rootBBOX = root.get_child_optional("bbox");
	if (!rootBBOX) {
		repoError << "No root bounding box specified.";
		errMsg = REPOERR_MODEL_FILE_READ;
		return nullptr;
	}
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
	trans_matrix_map.push_back(transMat);
	transformations.insert(rootNode);

	// Process children of root node 
	char comma;
	for (long i = 0; i < file_meta.numChildren; i++)
	{
		if (i % 500 == 0 || i == file_meta.numChildren - 1)
		{
			repoInfo << "Importing " << i << " of " << file_meta.numChildren << " JSON nodes";
		}
		fin->read(&comma, 1);
		boost::property_tree::ptree jsonTree = getNextJSON(sizes[i + 2]);
		createObject(jsonTree);
	}

	// Attach all the parents to the materials
	repoInfo << "Attaching materials to parents";
	materials.clear();
	for (int i = 0; i < matNodeList.size(); i++)
	{
		repo::core::model::MaterialNode *tmpMaterial = new repo::core::model::MaterialNode();
		*tmpMaterial = matNodeList[i]->cloneAndAddParent(matParents[i]);
		materials.insert(tmpMaterial);
	}

	// Preparing reference files
	std::vector<std::string> fileVect;
	if (!orgFile.empty())
	{
		fileVect.push_back(orgFile);
	}

	// Processing meshes
	repo::core::model::RepoNodeSet meshes;
	if (!offset.size()) { offset = { 0, 0, 0 }; }
	for (const auto &entry : meshEntries) {
		std::vector<repo::lib::RepoVector3D> vertices;
		std::vector<std::vector<float>> boundingBox;
		// Offsetting all the verts by the world offset to reduce the magnitude 
		// of their values so they can be cast to floats (widely used in 3D libs)
		for (const auto &v : entry.rawVertices) {
			repo::lib::RepoVector3D v32 = { (float)(v.x - offset[0]), (float)(v.y - offset[1]), (float)(v.z - offset[2]) };
			vertices.push_back(v32);
		}
		for (const auto &bbArr : entry.boundingBox) {
			std::vector<float> bb;
			for (int i = 0; i < bbArr.size(); ++i) {
				bb.push_back((float)bbArr[i] - offset[i]);
			}
			boundingBox.push_back(bb);
		}
		auto mesh = repo::core::model::RepoBSONFactory::makeMeshNode(vertices, entry.faces, entry.normals, boundingBox, { entry.parent });
		repo::core::model::RepoBSONBuilder builder;
		builder.append(REPO_NODE_LABEL_SHARED_ID, entry.sharedID);
		auto changes = builder.obj();
		meshes.insert(new repo::core::model::MeshNode(mesh.cloneAndAddFields(&changes, false)));
	}

	// Generate scene
	repo::core::model::RepoScene * scenePtr = new repo::core::model::RepoScene(fileVect, cameras, meshes, materials, metadata, textures, transformations);
	scenePtr->setWorldOffset(offset);
	
	// Cleanup
	delete[] dataBuffer;
	finCompressed->close();
	delete fin;
	delete inbuf;
	delete finCompressed;

	return scenePtr;
}