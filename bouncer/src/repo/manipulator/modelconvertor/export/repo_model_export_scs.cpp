#include "repo_model_export_scs.h"

#include "sc_store.h"
#include "sc_assemblytree.h"
#include "hoops_license.h"

#include "repo/core/model/bson/repo_node_mesh.h"
#include "repo/core/model/bson/repo_node_material.h"
#include "repo/core/model/bson/repo_node_texture.h"

using namespace repo::lib;
using namespace repo::manipulator::modelconvertor;

#pragma optimize("", off)

class Logger : public SC::Store::Logger
{
	void Message(char const* msg) const override
	{
		repoInfo << msg;
	}
};

SC::Store::Color colour(repo::lib::repo_color3d_t c)
{
	return SC::Store::Color(c.r, c.g, c.b, 1);
}

ScsExport::ScsExport(repo::core::model::RepoScene* scene)
	:WebModelExport(scene)
{
	SC::Store::Database::SetLicense(HOOPS_LICENSE);
	Logger logger;
	SC::Store::Cache cache = SC::Store::Database::Open(logger);

	auto cacheDirectory = std::string("D:/3drepo/3drepobouncer_staging/") + repo::lib::RepoUUID::createUUID().toString();

	SC::Store::Model model = cache.Open(cacheDirectory.c_str());
	SC::Store::InclusionKey include_key = model.Include(model);
	SC::Store::AssemblyTree tree(logger);

	SC::Store::NodeId root;
	tree.CreateAssemblyTreeRoot(root);

	tree.AddAttribute(root, "container", SC::Store::AssemblyTree::AttributeType::AttributeTypeString, scene->getProjectName().c_str());
	tree.AddAttribute(root, "database", SC::Store::AssemblyTree::AttributeType::AttributeTypeString, scene->getDatabaseName().c_str());

	std::unordered_map<repo::lib::RepoUUID, SC::Store::ImageKey, repo::lib::RepoUUIDHasher> materialTexture;
	for (auto n : scene->getAllTextures(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		auto node = dynamic_cast<repo::core::model::TextureNode*>(n);

		SC::Store::ImageFormat format;

		if (node->getFileExtension() == "png") {
			format = SC::Store::ImageFormat::PNG;
		}
		else if (node->getFileExtension() == "jpg") {
			format = SC::Store::ImageFormat::JPEG;
		}
		else if (node->getFileExtension() == "bmp") {
			format = SC::Store::ImageFormat::BMP;
		}
		else {
			repoInfo << "Unsupported image format";
			continue;
		}
		
		auto imageKey = model.Insert(node->getRawData().size(), node->getRawData().data(), format);

		for (auto& p : node->getParentIDs()) {
			materialTexture[p] = imageKey;
		}
	}

	std::unordered_map<repo::lib::RepoUUID, SC::Store::MaterialKey, repo::lib::RepoUUIDHasher> meshMaterials;
	for (auto n : scene->getAllMaterials(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		auto node = dynamic_cast<repo::core::model::MaterialNode*>(n);
		auto src = node->getMaterialStruct();
		
		SC::Store::Material dst;
		dst.ambient_color = colour(src.ambient);
		dst.diffuse_color = colour(src.diffuse);
		dst.diffuse_color.alpha = src.opacity;
	//	dst.emissive_color = colour(src.emissive);
		dst.specular_color = colour(src.specular);
		dst.properties.mirror = 0;

		auto textureIt = materialTexture.find(node->getSharedID());
		if (textureIt != materialTexture.end()) {
			dst.diffuse_textures.push_back(SC::Store::Texture(textureIt->second));
			dst.diffuse_color.alpha = 1;
		}

		auto key = model.Insert(dst);

		for (auto& p : node->getParentIDs()) {
			meshMaterials[p] = key;
		}
	}

	for (auto m : scene->getAllMeshes(repo::core::model::RepoScene::GraphType::DEFAULT))
	{
		auto node = dynamic_cast<repo::core::model::MeshNode*>(m);

		if (node->getPrimitive() != repo::core::model::MeshNode::Primitive::TRIANGLES)
		{
			continue;
		}

		SC::Store::Mesh mesh;
		mesh.point_count = node->getNumVertices();
		mesh.points = (SC::Store::Point const*)node->getVertices().data();

		size_t attributeCount = 1;

		if (node->getNormals().size()) {
			mesh.normal_count = node->getNumVertices();
			mesh.normals = (SC::Store::Normal const*)node->getNormals().data();
			attributeCount++;
		}

		for (auto& channel : node->getUVChannelsSeparated())
		{
			mesh.uv_count = node->getNumVertices();
			mesh.uvs = (SC::Store::UV const*)channel.data();
			attributeCount++;

			break; // SCS supports only one uv channel
		}

		mesh.face_elements.resize(node->getNumFaces());
		const auto& faces = node->getFaces();
		for (auto f = 0; f < mesh.face_elements.size(); f++)
		{
			auto& face = faces[f];
			auto& element = mesh.face_elements[f];

			for (auto i = 0; i < 3; i++) {
				for (auto a = 0; a < attributeCount; a++) { // Point, (and optional...) Normal, UV, Colour, in that order.
					element.indices.push_back(face[i]);
				}
			}
		}

		mesh.flags = (SC::Store::Mesh::Bits)(mesh.flags | SC::Store::Mesh::Bits::TwoSided);

		SC::Store::MeshKey meshKey = model.Insert(mesh);

		auto instanceKey = model.Instance(meshKey, {}, meshMaterials[node->getSharedID()]);

		SC::Store::NodeId nodeId;
		tree.CreateAndAddBodyInstance(root, nodeId);
		tree.SetBodyInstanceMeshInstanceKey(nodeId, SC::Store::InstanceInc(include_key, instanceKey));
		tree.SetGenericId(nodeId, node->getUniqueID().toString().c_str());
	}

	tree.SerializeToModel(model);

	model.PrepareStream();
	model.GenerateSCSFile("D:/3drepo/3drepo.io/public/mymodel.scs");
}

repo::lib::repo_web_buffers_t ScsExport::getAllFilesExportedAsBuffer() const
{
	return buffers;
}