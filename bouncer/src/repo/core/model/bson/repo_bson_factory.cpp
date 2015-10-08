#include "repo_bson_factory.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "../../../lib/repo_log.h"

using namespace repo::core::model;



uint64_t RepoBSONFactory::appendDefaults(
	RepoBSONBuilder &builder,
	const std::string &type,
	const unsigned int api,
	const repoUUID &sharedId,
	const std::string &name,
	const std::vector<repoUUID> &parents,
	const repoUUID &uniqueID)
{

	uint64_t bytesize = 0;

	//--------------------------------------------------------------------------
	// ID field (UUID)
	builder.append(REPO_NODE_LABEL_ID, uniqueID);

	//--------------------------------------------------------------------------
	// Shared ID (UUID)
	builder.append(REPO_NODE_LABEL_SHARED_ID, sharedId);


	bytesize += 2 * sizeof(repoUUID);

	//--------------------------------------------------------------------------
	// Type
	if (!type.empty())
	{
		builder << REPO_NODE_LABEL_TYPE << type;
		bytesize += sizeof(type);
	}


	//--------------------------------------------------------------------------
	// API level
	builder << REPO_NODE_LABEL_API << api;

	bytesize += sizeof(api);

	//--------------------------------------------------------------------------
	// Parents
	if (parents.size() > 0)
	{
		builder.appendArray(REPO_NODE_LABEL_PARENTS, builder.createArrayBSON(parents));

		bytesize += parents.size() * sizeof(parents[0]);
	}

	//--------------------------------------------------------------------------
	// Name
	if (!name.empty())
	{
		builder << REPO_NODE_LABEL_NAME << name;
		bytesize += sizeof(name);
	}

	return bytesize;
}

RepoNode RepoBSONFactory::makeRepoNode(std::string type)
{
	RepoBSONBuilder builder;
	appendDefaults(builder, type);
	return RepoNode(builder.obj());
}

CameraNode RepoBSONFactory::makeCameraNode(
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
	RepoBSONBuilder builder;

	//--------------------------------------------------------------------------
	// Compulsory fields such as _id, type, api as well as path
	// and optional name
	appendDefaults(builder, REPO_NODE_TYPE_CAMERA, apiLevel, generateUUID(), name);

	//--------------------------------------------------------------------------
	// Aspect ratio
	builder << REPO_NODE_LABEL_ASPECT_RATIO << aspectRatio;

	//--------------------------------------------------------------------------
	// Far clipping plane
	builder << REPO_NODE_LABEL_FAR << farClippingPlane;

	//--------------------------------------------------------------------------
	// Near clipping plane
	builder << REPO_NODE_LABEL_NEAR << nearClippingPlane;

	//--------------------------------------------------------------------------
	// Field of view
	builder << REPO_NODE_LABEL_FOV << fieldOfView;

	//--------------------------------------------------------------------------
	// Look at vector
	builder.appendVector(REPO_NODE_LABEL_LOOK_AT, lookAt);

	//--------------------------------------------------------------------------
	// Position vector
	builder.appendVector(REPO_NODE_LABEL_POSITION, position);

	//--------------------------------------------------------------------------
	// Up vector
	builder.appendVector(REPO_NODE_LABEL_UP, up);

	return CameraNode(builder.obj());
}


MaterialNode RepoBSONFactory::makeMaterialNode(
	const repo_material_t &material,
	const std::string     &name,
	const int             &apiLevel)
{
	RepoBSONBuilder builder;

	// Compulsory fields such as _id, type, api as well as path
	// and optional name
	appendDefaults(builder, REPO_NODE_TYPE_MATERIAL, apiLevel, generateUUID(), name);

	if (material.ambient.size() > 0)
		builder.appendArray(REPO_NODE_MATERIAL_LABEL_AMBIENT, builder.createArrayBSON(material.ambient));
	if (material.diffuse.size() > 0)
		builder.appendArray(REPO_NODE_MATERIAL_LABEL_DIFFUSE, builder.createArrayBSON(material.diffuse));
	if (material.specular.size() > 0)
		builder.appendArray(REPO_NODE_MATERIAL_LABEL_SPECULAR, builder.createArrayBSON(material.specular));
	if (material.emissive.size() > 0)
		builder.appendArray(REPO_NODE_MATERIAL_LABEL_EMISSIVE, builder.createArrayBSON(material.emissive));


	if (material.isWireframe)
		builder << REPO_NODE_MATERIAL_LABEL_WIREFRAME << material.isWireframe;
	if (material.isTwoSided)
		builder << REPO_NODE_MATERIAL_LABEL_TWO_SIDED << material.isTwoSided;

	if (material.opacity == material.opacity)
		builder << REPO_NODE_MATERIAL_LABEL_OPACITY << material.opacity;

	if (material.shininess == material.shininess)
		builder << REPO_NODE_MATERIAL_LABEL_SHININESS << material.shininess;

	if (material.shininessStrength == material.shininessStrength)
		builder << REPO_NODE_MATERIAL_LABEL_SHININESS_STRENGTH << material.shininessStrength;

	return MaterialNode(builder.obj());
}

MapNode RepoBSONFactory::makeMapNode(
	const uint32_t        &width,
	const uint32_t        &zoom,
	const float           &tilt,
	const float           &tileSize,
	const float           &longitude,
	const float           &latitude,
	const repo_vector_t   &centrePoint,
	const std::string     &name,
	const int             &apiLevel)
{
	RepoBSONBuilder map_builder;

	// Compulsory fields such as _id, type, api as well as path
	// and optional name
	appendDefaults(map_builder, REPO_NODE_TYPE_MAP, apiLevel, generateUUID(), name, std::vector<repoUUID>());
	//--------------------------------------------------------------------------
	// width (# of horizontal tiles)
	map_builder << REPO_NODE_MAP_LABEL_WIDTH << width;
	//--------------------------------------------------------------------------
	// yrot (tilt on y axis in radians)
	map_builder << REPO_NODE_MAP_LABEL_YROT << tilt;
	//--------------------------------------------------------------------------
	// world tile size (size of tiles)
	map_builder << REPO_NODE_MAP_LABEL_TILESIZE << tileSize;
	//--------------------------------------------------------------------------
	// longitude
	map_builder << REPO_NODE_MAP_LABEL_LONG << longitude;
	//--------------------------------------------------------------------------
	// latitude
	map_builder << REPO_NODE_MAP_LABEL_LAT << latitude;
	//--------------------------------------------------------------------------
	// zoom
	map_builder << REPO_NODE_MAP_LABEL_ZOOM << zoom;
	//--------------------------------------------------------------------------
	// trans
	map_builder << REPO_NODE_MAP_LABEL_TRANS << BSON_ARRAY(centrePoint.x << centrePoint.y << centrePoint.z);
	//--------------------------------------------------------------------------
	return map_builder.obj();
}

MetadataNode RepoBSONFactory::makeMetaDataNode(
	RepoBSON			          &metadata,
	const std::string             &mimeType,
	const std::string             &name,
	const std::vector<repoUUID>  &parents,
	const int                     &apiLevel)
{
	RepoBSONBuilder builder;

	// Compulsory fields such as _id, type, api as well as path
	// and optional name
	appendDefaults(builder, REPO_NODE_TYPE_METADATA, apiLevel, generateUUID(), name, parents);

	//--------------------------------------------------------------------------
	// Media type
	if (!mimeType.empty())
		builder << REPO_LABEL_MEDIA_TYPE << mimeType;

	//--------------------------------------------------------------------------
	// Add metadata subobject
	if (!metadata.isEmpty())
		builder << REPO_NODE_LABEL_METADATA << metadata;

	return MetadataNode(builder.obj());
}

MetadataNode RepoBSONFactory::makeMetaDataNode(
	const std::vector<std::string>  &keys,
	const std::vector<std::string>  &values,
	const std::string               &name,
	const std::vector<repoUUID>     &parents,
	const int                       &apiLevel)
{
	RepoBSONBuilder builder;
	// Compulsory fields such as _id, type, api as well as path
	// and optional name
	appendDefaults(builder, REPO_NODE_TYPE_METADATA, apiLevel, generateUUID(), name, parents);

	//check keys and values have the same sizes

	if (keys.size() != values.size())
	{
		repoWarning << "makeMetaDataNode: number of keys (" << keys.size()
			<< ") does not match the number of values(" << values.size() << ")!";
	}

	std::vector<std::string>::const_iterator kit = keys.begin();
	std::vector<std::string>::const_iterator vit = values.begin();
	for (; kit != keys.end() && vit != values.end(); ++kit, ++vit)
	{

		std::string key = *kit;
		std::string value = *vit;

		if (!key.empty() && !value.empty())
		{
			//Check if it is a number, if it is, store it as a number

			try{
				long long valueInt = boost::lexical_cast<long long>(value);
				builder << key << valueInt;
			}
			catch (boost::bad_lexical_cast &)
			{
				//not an int, try a double

				try{
					double valueFloat = boost::lexical_cast<double>(value);
					builder << key << valueFloat;
				}
				catch (boost::bad_lexical_cast &)
				{
					//not an int or float, store as string
					builder << key << value;

				}
			}
		}

	}

	return MetadataNode(builder.obj());
}

MeshNode RepoBSONFactory::makeMeshNode(
	std::vector<repo_vector_t>                  &vertices,
	std::vector<repo_face_t>                    &faces,
	std::vector<repo_vector_t>                  &normals,
	std::vector<std::vector<float>>             &boundingBox,
	std::vector<std::vector<repo_vector2d_t>>   &uvChannels,
	std::vector<repo_color4d_t>                 &colors,
	std::vector<std::vector<float>>             &outline,
	const std::string                           &name,
	const int                                   &apiLevel)
{
	RepoBSONBuilder builder;
	uint64_t bytesize = 0; //track the (approximate) size to know when we need to offload to gridFS
	repoUUID uniqueID = generateUUID();
	bytesize += appendDefaults(builder, REPO_NODE_TYPE_MESH, apiLevel, generateUUID(), name, std::vector<repoUUID>(), uniqueID);

	std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> binMapping;

	if (boundingBox.size() > 0)
	{
		RepoBSONBuilder arrayBuilder;

		for (int i = 0; i < boundingBox.size(); i++)
		{
			arrayBuilder.appendArray(std::to_string(i), builder.createArrayBSON(boundingBox[i]));
			bytesize += boundingBox[i].size() * sizeof(boundingBox[i][0]);
		}

		builder.appendArray(REPO_NODE_MESH_LABEL_BOUNDING_BOX, arrayBuilder.obj());

	}


	if (outline.size() > 0)
	{
		RepoBSONBuilder arrayBuilder;

		for (int i = 0; i < outline.size(); i++)
		{
			arrayBuilder.appendArray(boost::lexical_cast<std::string>(i), builder.createArrayBSON(outline[i]));
			bytesize += outline[i].size() * sizeof(outline[i][0]);
		}

		builder.appendArray(REPO_NODE_MESH_LABEL_OUTLINE, arrayBuilder.obj());
	}

	/*
	* TODO: because mongo has a stupid internal limit of 64MB, we can't store everything in a BSON
	* There are 2 options
	* 1. store binaries in memory outside of the bson and put it into GRIDFS at the point of commit
	* 2. leave mongo's bson, use our own/exteral library that doesn't have this limit and database handle this at the point of commit
	* below uses option 1, but ideally we should be doing option 2.
	*/

	if (vertices.size() > 0)
	{
		uint64_t verticesByteCount = vertices.size() * sizeof(vertices[0]);

		if (verticesByteCount + bytesize >= REPO_BSON_MAX_BYTE_SIZE)
		{
			std::string bName = UUIDtoString(uniqueID) + "_vertices";
			//inclusion of this binary exceeds the maximum, store separately
			binMapping[REPO_NODE_MESH_LABEL_VERTICES] =
				std::pair<std::string, std::vector<uint8_t>>(bName, std::vector<uint8_t>());
			binMapping[REPO_NODE_MESH_LABEL_VERTICES].second.resize(verticesByteCount); //uint8_t will ensure it is a byte addrressing
			memcpy(binMapping[REPO_NODE_MESH_LABEL_VERTICES].second.data(), &vertices[0], verticesByteCount);
			bytesize += sizeof(bName);
		}
		else
		{

			builder.appendBinary(
				REPO_NODE_MESH_LABEL_VERTICES,
				&vertices[0],
				vertices.size() * sizeof(vertices[0]),
				REPO_NODE_MESH_LABEL_VERTICES_BYTE_COUNT,
				REPO_NODE_MESH_LABEL_VERTICES_COUNT
				);
			bytesize += verticesByteCount;
		}

	}

	if (faces.size() > 0)
	{
		builder << REPO_NODE_MESH_LABEL_FACES_COUNT << (uint32_t)(faces.size());

		// In API LEVEL 1, faces are stored as
		// [n1, v1, v2, ..., n2, v1, v2...]
		std::vector<repo_face_t>::iterator faceIt;

		std::vector<uint32_t> facesLevel1;
		for (auto &face : faces){
			facesLevel1.push_back(face.numIndices);
			for (uint32_t ind = 0; ind < face.numIndices; ind++)
			{
				facesLevel1.push_back(face.indices[ind]);
			}
		}


		uint64_t facesByteCount = facesLevel1.size() * sizeof(facesLevel1[0]);

		if (facesByteCount + bytesize >= REPO_BSON_MAX_BYTE_SIZE)
		{
			std::string bName = UUIDtoString(uniqueID) + "_faces";
			//inclusion of this binary exceeds the maximum, store separately
			binMapping[REPO_NODE_MESH_LABEL_FACES] =
				std::pair<std::string, std::vector<uint8_t>>(bName, std::vector<uint8_t>());
			binMapping[REPO_NODE_MESH_LABEL_FACES].second.resize(facesByteCount); //uint8_t will ensure it is a byte addrressing
			memcpy(binMapping[REPO_NODE_MESH_LABEL_FACES].second.data(), &facesLevel1[0], facesByteCount);

			bytesize += sizeof(bName);
		}
		else
		{
			builder.appendBinary(
				REPO_NODE_MESH_LABEL_FACES,
				&facesLevel1[0],
				facesLevel1.size() * sizeof(facesLevel1[0]),
				REPO_NODE_MESH_LABEL_FACES_BYTE_COUNT
				);

			bytesize += facesByteCount;
		}

	}

	if (normals.size() > 0)
	{

		uint64_t normalsByteCount = normals.size() * sizeof(normals[0]);

		if (normalsByteCount + bytesize >= REPO_BSON_MAX_BYTE_SIZE)
		{
			std::string bName = UUIDtoString(uniqueID) + "_normals";
			//inclusion of this binary exceeds the maximum, store separately
			binMapping[REPO_NODE_MESH_LABEL_NORMALS] =
				std::pair<std::string, std::vector<uint8_t>>(bName, std::vector<uint8_t>());
			binMapping[REPO_NODE_MESH_LABEL_NORMALS].second.resize(normalsByteCount); //uint8_t will ensure it is a byte addrressing
			memcpy(binMapping[REPO_NODE_MESH_LABEL_NORMALS].second.data(), &normals[0], normalsByteCount);

			bytesize += sizeof(bName);
		}
		else
		{
			builder.appendBinary(
				REPO_NODE_MESH_LABEL_NORMALS,
				&normals[0],
				normals.size() * sizeof(normals[0]));

			bytesize += normalsByteCount;
		}
	}

	//if (!vertexHash.empty())
	//{
	//	// TODO: Fix this call - needs to be fixed as int conversion is overloaded
	//	//builder << REPO_NODE_LABEL_SHA256 << (long unsigned int)(vertexHash);
	//}


	//--------------------------------------------------------------------------
	// Vertex colors
	if (colors.size())
	{
		uint64_t colorsByteCount = colors.size() * sizeof(colors[0]);

		if (colorsByteCount + bytesize >= REPO_BSON_MAX_BYTE_SIZE)
		{
			std::string bName = UUIDtoString(uniqueID) + "_colors";
			//inclusion of this binary exceeds the maximum, store separately
			binMapping[REPO_NODE_MESH_LABEL_COLORS] =
				std::pair<std::string, std::vector<uint8_t>>(bName, std::vector<uint8_t>());
			binMapping[REPO_NODE_MESH_LABEL_COLORS].second.resize(colorsByteCount); //uint8_t will ensure it is a byte addrressing
			memcpy(binMapping[REPO_NODE_MESH_LABEL_COLORS].second.data(), &colors[0], colorsByteCount);

			bytesize += sizeof(bName);
		}
		else
		{
			builder.appendBinary(
				REPO_NODE_MESH_LABEL_COLORS,
				&colors[0],
				colors.size() * sizeof(colors[0]));
			bytesize += colorsByteCount;
		}

	}

	//--------------------------------------------------------------------------
	// UV channels
	if (uvChannels.size() > 0)
	{
		// Could be unsigned __int64 if BSON had such construct (the closest is only __int64)
		builder << REPO_NODE_MESH_LABEL_UV_CHANNELS_COUNT << (uint32_t)(uvChannels.size());

		std::vector<repo_vector2d_t> concatenated;

		std::vector<std::vector<repo_vector2d_t>>::iterator it;
		for (it = uvChannels.begin(); it != uvChannels.end(); ++it)
		{
			std::vector<repo_vector2d_t> channel = *it;

			std::vector<repo_vector2d_t>::iterator cit;
			for (cit = channel.begin(); cit != channel.end(); ++cit)
			{
				concatenated.push_back(*cit);
			}
		}


		uint64_t uvByteCount = concatenated.size() * sizeof(concatenated[0]);

		if (uvByteCount + bytesize >= REPO_BSON_MAX_BYTE_SIZE)
		{
			std::string bName = UUIDtoString(uniqueID) + "_uv";
			//inclusion of this binary exceeds the maximum, store separately
			binMapping[REPO_NODE_MESH_LABEL_UV_CHANNELS] =
				std::pair<std::string, std::vector<uint8_t>>(bName, std::vector<uint8_t>());
			binMapping[REPO_NODE_MESH_LABEL_UV_CHANNELS].second.resize(uvByteCount); //uint8_t will ensure it is a byte addrressing
			memcpy(binMapping[REPO_NODE_MESH_LABEL_UV_CHANNELS].second.data(), &concatenated[0], uvByteCount);

			bytesize += sizeof(bName);
		}
		else
		{
			builder.appendBinary(
				REPO_NODE_MESH_LABEL_UV_CHANNELS,
				&concatenated[0],
				concatenated.size() * sizeof(concatenated[0]),
				REPO_NODE_MESH_LABEL_UV_CHANNELS_BYTE_COUNT);

			bytesize += uvByteCount;
		}
	}

	return MeshNode(builder.obj(), binMapping);
}

RepoProjectSettings RepoBSONFactory::makeRepoProjectSettings(
	const std::string &uniqueProjectName,
	const std::string &owner,
	const std::string &group,
	const std::string &type,
	const std::string &description,
	const uint8_t     &ownerPermissionsOctal,
	const uint8_t     &groupPermissionsOctal,
	const uint8_t     &publicPermissionsOctal)
{
	RepoBSONBuilder builder;

	//--------------------------------------------------------------------------
	// Project name
	if (!uniqueProjectName.empty())
		builder << REPO_LABEL_ID << uniqueProjectName;

	//--------------------------------------------------------------------------
	// Owner
	if (!owner.empty())
		builder << REPO_LABEL_OWNER << owner;

	//--------------------------------------------------------------------------
	// Description
	if (!description.empty())
		builder << REPO_LABEL_DESCRIPTION << description;

	//--------------------------------------------------------------------------
	// Type
	if (!type.empty())
		builder << REPO_LABEL_TYPE << type;

	//--------------------------------------------------------------------------
	// Group
	if (!group.empty())
		builder << REPO_LABEL_GROUP << group;

	//--------------------------------------------------------------------------
	// Permissions
	mongo::BSONArrayBuilder arrayBuilder;
	arrayBuilder << ownerPermissionsOctal;
	arrayBuilder << groupPermissionsOctal;
	arrayBuilder << publicPermissionsOctal;
	builder << REPO_LABEL_PERMISSIONS << arrayBuilder.arr();

	//--------------------------------------------------------------------------
	// Add to the parent object
	return RepoProjectSettings(builder.obj());
}

RepoUser RepoBSONFactory::makeRepoUser(
	const std::string                           &userName,
	const std::string                           &password,
	const std::string                           &firstName,
	const std::string                           &lastName,
	const std::string                           &email,
	const std::list<std::pair<std::string, std::string>>  &projects,
	const std::list<std::pair<std::string, std::string>>   &roles,
	const std::list<std::pair<std::string, std::string>>   &groups,
	const std::list<std::pair<std::string, std::string>>   &apiKeys,
	const std::vector<char>                     &avatar)
{
	RepoBSONBuilder builder;
	RepoBSONBuilder customDataBuilder;

	builder.append(REPO_LABEL_ID, generateUUID());
	if (!userName.empty())
		builder << REPO_USER_LABEL_USER << userName;

	if (!password.empty())
	{
		RepoBSONBuilder credentialsBuilder;
		credentialsBuilder << REPO_USER_LABEL_CLEARTEXT << password;
		builder << REPO_USER_LABEL_CREDENTIALS << credentialsBuilder.obj();
	}

	if (!firstName.empty())
		customDataBuilder << REPO_USER_LABEL_FIRST_NAME << firstName;

	if (!lastName.empty())
		customDataBuilder << REPO_USER_LABEL_LAST_NAME << lastName;

	if (!email.empty())
		customDataBuilder << REPO_USER_LABEL_EMAIL << email;

	if (projects.size())
		customDataBuilder.appendArrayPair(REPO_USER_LABEL_PROJECTS, projects, REPO_USER_LABEL_OWNER, REPO_USER_LABEL_PROJECT);

	if (groups.size())
		customDataBuilder.appendArrayPair(REPO_USER_LABEL_GROUPS, groups, REPO_USER_LABEL_OWNER, REPO_USER_LABEL_GROUP);

	if (!apiKeys.empty())
		customDataBuilder.appendArrayPair(REPO_USER_LABEL_API_KEYS, apiKeys, REPO_USER_LABEL_LABEL, REPO_USER_LABEL_KEY);

	if (avatar.size())
	{
		RepoBSONBuilder avatarBuilder;
		//FIXME: use repo image?
		avatarBuilder.appendBinary(REPO_LABEL_DATA, &avatar.at(0), sizeof(avatar.at(0))*avatar.size());
		customDataBuilder << REPO_LABEL_AVATAR << avatarBuilder.obj();
	}


	builder << REPO_USER_LABEL_CUSTOM_DATA << customDataBuilder.obj();

	if (roles.size())
		builder.appendArrayPair(REPO_USER_LABEL_ROLES, roles, REPO_USER_LABEL_DB, REPO_USER_LABEL_ROLE);

	return RepoUser(builder.obj());
}


ReferenceNode RepoBSONFactory::makeReferenceNode(
	const std::string &database,
	const std::string &project,
	const repoUUID    &revisionID,
	const bool        &isUniqueID,
	const std::string &name,
	const int         &apiLevel)
{
	RepoBSONBuilder builder;
	std::string nodeName = name.empty() ? database + "." + project : name;

	appendDefaults(builder, REPO_NODE_TYPE_REFERENCE, apiLevel, generateUUID(), nodeName);

	//--------------------------------------------------------------------------
	// Project owner (company or individual)
	if (!database.empty())
		builder << REPO_NODE_REFERENCE_LABEL_OWNER << database;

	//--------------------------------------------------------------------------
	// Project name
	if (!project.empty())
		builder << REPO_NODE_REFERENCE_LABEL_PROJECT << project;

	//--------------------------------------------------------------------------
	// Revision ID (specific revision if UID, branch if SID)
	builder.append(
		REPO_NODE_REFERENCE_LABEL_REVISION_ID,
		revisionID);

	//--------------------------------------------------------------------------
	// Unique set if the revisionID is UID, not set if SID (branch)
	if (isUniqueID)
		builder << REPO_NODE_REFERENCE_LABEL_UNIQUE << isUniqueID;

	return ReferenceNode(builder.obj());
}

RevisionNode RepoBSONFactory::makeRevisionNode(
	const std::string			   &user,
	const repoUUID                 &branch,
	const std::vector<repoUUID>    &currentNodes,
	const std::vector<repoUUID>    &added,
	const std::vector<repoUUID>    &removed,
	const std::vector<repoUUID>    &modified,
	const std::vector<std::string> &files,
	const std::vector<repoUUID>    &parent,
	const std::string              &message,
	const std::string              &tag,
	const int                      &apiLevel
	)
{
	RepoBSONBuilder builder;
	repoUUID uniqueID = generateUUID();

	//--------------------------------------------------------------------------
	// Compulsory fields such as _id, type, api as well as path
	appendDefaults(builder, REPO_NODE_TYPE_REVISION, apiLevel, branch, "", parent, uniqueID);

	//--------------------------------------------------------------------------
	// Author
	if (!user.empty())
		builder << REPO_NODE_REVISION_LABEL_AUTHOR << user;

	//--------------------------------------------------------------------------
	// Message
	if (!message.empty())
		builder << REPO_NODE_REVISION_LABEL_MESSAGE << message;

	//--------------------------------------------------------------------------
	// Tag
	if (!tag.empty())
		builder << REPO_NODE_REVISION_LABEL_TAG << tag;

	//--------------------------------------------------------------------------
	// Timestamp
	builder.appendTimeStamp(REPO_NODE_REVISION_LABEL_TIMESTAMP);

	//--------------------------------------------------------------------------

	// Current Unique IDs
	if (currentNodes.size() > 0)
		builder.appendArray(REPO_NODE_REVISION_LABEL_CURRENT_UNIQUE_IDS, builder.createArrayBSON(currentNodes));

	//--------------------------------------------------------------------------
	// Added Shared IDs

	if (added.size() > 0)
		builder.appendArray(REPO_NODE_REVISION_LABEL_ADDED_SHARED_IDS, builder.createArrayBSON(added));

	//--------------------------------------------------------------------------
	// Deleted Shared IDs
	if (removed.size() > 0)
		builder.appendArray(REPO_NODE_REVISION_LABEL_DELETED_SHARED_IDS, builder.createArrayBSON(removed));

	//--------------------------------------------------------------------------
	// Modified Shared IDs
	if (modified.size() > 0)
		builder.appendArray(REPO_NODE_REVISION_LABEL_MODIFIED_SHARED_IDS, builder.createArrayBSON(modified));

	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// original files references
	if (files.size() > 0)
	{

		std::string uniqueIDStr = UUIDtoString(uniqueID);
		mongo::BSONObjBuilder arrbuilder;
		for (int i = 0; i < files.size(); ++i)
		{
			arrbuilder << std::to_string(i) << uniqueIDStr + files[i];
		}

		builder.appendArray(REPO_NODE_REVISION_LABEL_REF_FILE, arrbuilder.obj());
	}

	return RevisionNode(builder.obj());
}

TextureNode RepoBSONFactory::makeTextureNode(
	const std::string &name,
	const char        *data,
	const uint32_t    &byteCount,
	const uint32_t    &width,
	const uint32_t    &height,
	const int         &apiLevel)
{
	RepoBSONBuilder builder;
	appendDefaults(builder, REPO_NODE_TYPE_TEXTURE, apiLevel, generateUUID(), name);
	//
	// Width
	//
	builder << REPO_LABEL_WIDTH << width;

	//
	// Height
	//
	builder << REPO_LABEL_HEIGHT << height;

	//
	// Format TODO: replace format with MIME Type?
	//
	if (name.empty())
	{
		boost::filesystem::path file{ name };
		builder << REPO_NODE_LABEL_EXTENSION << file.extension().c_str();
	}

	//
	// Data
	//

	if (NULL != data && byteCount > 0)
		builder.appendBinary(
		REPO_LABEL_DATA,
		data,
		byteCount,
		REPO_NODE_LABEL_DATA_BYTE_COUNT);

	return TextureNode(builder.obj());
}

TransformationNode RepoBSONFactory::makeTransformationNode(
	const std::vector<std::vector<float>> &transMatrix,
	const std::string                     &name,
	const std::vector<repoUUID>		  &parents,
	const int                             &apiLevel)
{
	RepoBSONBuilder builder;

	appendDefaults(builder, REPO_NODE_TYPE_TRANSFORMATION, apiLevel, generateUUID(), name, parents);

	//--------------------------------------------------------------------------
	// Store matrix as array of arrays
	uint32_t matrixSize = 4;
	RepoBSONBuilder rows;
	for (uint32_t i = 0; i < transMatrix.size(); ++i)
	{
		RepoBSONBuilder columns;
		for (uint32_t j = 0; j < transMatrix[i].size(); ++j){
			columns << std::to_string(j) << transMatrix[i][j];
		}
		rows.appendArray(std::to_string(i), columns.obj());
	}
	builder.appendArray(REPO_NODE_LABEL_MATRIX, rows.obj());

	return TransformationNode(builder.obj());
}
