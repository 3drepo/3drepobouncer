#include "repo_bson_factory.h"

using namespace repo::core::model::bson;

RepoNode* RepoBSONFactory::makeRepoNode(std::string type){
	return RepoNode::createRepoNode(type);
}

CameraNode* RepoBSONFactory::makeCameraNode(
	const float         &aspectRatio,
	const float         &farClippingPlane,
	const float         &nearClippingPlane,
	const float         &fieldOfView,
	const repo_vector_t &lookAt,
	const repo_vector_t &position,
	const repo_vector_t &up,
	const std::string   &name,
	const int           &apiLevel)
{
	return CameraNode::createCameraNode(aspectRatio, farClippingPlane, nearClippingPlane,
		fieldOfView, lookAt, position, up, apiLevel, name);
}


MaterialNode* RepoBSONFactory::makeMaterialNode(
	const repo_material_t &material,
	const std::string     &name,
	const int             &apiLevel)
{
	return MaterialNode::createMaterialNode(material, name,  apiLevel);
}

TextureNode* RepoBSONFactory::makeTextureNode(
	const std::string &name,
	const char        *memblock,
	const uint32_t    &size,
	const uint32_t    &width,
	const uint32_t    &height,
	const int         &apiLevel)
{
	return TextureNode::createTextureNode(name, memblock, size, width, height, apiLevel);
}

MetadataNode* RepoBSONFactory::makeMetaDataNode(
	RepoBSON			          &metadata,
	const std::string             &mimeType,
	const std::string             &name,
	const std::vector<repo_uuid>  &parents,
	const int                     &apiLevel)
{
	return MetadataNode::createMetadataNode(metadata, mimeType, name, parents, apiLevel);
}

MeshNode* RepoBSONFactory::makeMeshNode(
	std::vector<repo_vector_t>                  &vertices,
	std::vector<repo_face_t>                    &faces,
	std::vector<repo_vector_t>                  &normals,
	std::vector<repo_vector_t>                  &boundingBox,
	std::vector<std::vector<repo_vector2d_t>>   &uvChannels,
	std::vector<repo_color4d_t>                 &colors,
	std::vector<repo_vector2d_t>                &outline,
	const std::string                           &name,
	const int                                   &apiLevel)
{
	return MeshNode::createMeshNode(vertices, faces, normals, boundingBox, 
		uvChannels, colors, outline, apiLevel, name);
}

TransformationNode* RepoBSONFactory::makeTransformationNode(
	const std::vector<std::vector<float>> &transMatrix,
	const std::string                     &name,
	const std::vector<repo_uuid>		  &parents,
	const int                             &apiLevel)
{
	return TransformationNode::createTransformationNode(transMatrix, name, parents, apiLevel);
}