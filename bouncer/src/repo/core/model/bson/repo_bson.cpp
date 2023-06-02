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

#include "repo_bson.h"

#include <mongo/client/dbclient.h>

using namespace repo::core::model;

RepoBSON::RepoBSON(const RepoBSON &obj,
	const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping) :
	mongo::BSONObj(obj),
	bigFiles(binMapping) {
	auto existingFiles = obj.getFilesMapping();

	for (const auto &pair : existingFiles) {
		if (bigFiles.find(pair.first) == bigFiles.end()) {
			bigFiles[pair.first] = pair.second;
		}
	}
}

RepoBSON::RepoBSON(
	const mongo::BSONObj &obj,
	const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping)
	: mongo::BSONObj(obj),
	bigFiles(binMapping)
{
}

int64_t RepoBSON::getCurrentTimestamp()
{
	mongo::Date_t date = mongo::Date_t(time(NULL) * 1000);
	return date.asInt64();
}

RepoBSON RepoBSON::fromJSON(const std::string &json)
{
	return RepoBSON(mongo::fromjson(json));
}

RepoBSON RepoBSON::cloneAndAddFields(
	const RepoBSON *changes) const
{
	mongo::BSONObjBuilder builder;

	builder.appendElementsUnique(*changes);

	builder.appendElementsUnique(*this);

	return RepoBSON(builder.obj());
}

RepoBSON RepoBSON::cloneAndShrink() const
{
	std::set<std::string> fields = getFieldNames();
	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>>> rawFiles(bigFiles.begin(), bigFiles.end());
	std::string uniqueIDStr = hasField(REPO_LABEL_ID) ? getUUIDField(REPO_LABEL_ID).toString() : repo::lib::RepoUUID::createUUID().toString();

	RepoBSON resultBson = *this;

	for (const std::string &field : fields)
	{
		if (getField(field).type() == ElementType::BINARY)
		{
			std::string fileName = uniqueIDStr + "_" + field;
			rawFiles[field] = std::pair<std::string, std::vector<uint8_t>>(fileName, std::vector<uint8_t>());
			getBinaryFieldAsVector(field, rawFiles[field].second);
			resultBson = resultBson.removeField(field);
		}
	}
	return RepoBSON(resultBson, rawFiles);
}

std::pair<repo::core::model::RepoBSON, std::vector<uint8_t>> RepoBSON::getBinariesAsBuffer() const {
	std::pair<repo::core::model::RepoBSON, std::vector<uint8_t>> res;
	if (bigFiles.size()) {
		std::vector<uint8_t> &buffer = res.second;
		mongo::BSONObjBuilder elemsBuilder;

		for (const auto &entry : bigFiles) {
			mongo::BSONObjBuilder entryBuilder;

			entryBuilder << REPO_LABEL_BINARY_START << (unsigned int)buffer.size();
			buffer.insert(buffer.end(), entry.second.second.begin(), entry.second.second.end());
			entryBuilder << REPO_LABEL_BINARY_SIZE << (unsigned int)entry.second.second.size();

			elemsBuilder << entry.first << entryBuilder.obj();
		}

		res.first = elemsBuilder.obj();
	}

	return res;
}

void RepoBSON::replaceBinaryWithReference(const repo::core::model::RepoBSON &fileRef, const repo::core::model::RepoBSON &elemRef) {
	mongo::BSONObjBuilder objBuilder;
	objBuilder << REPO_LABEL_BINARY_ELEMENTS << (mongo::BSONObj)elemRef;
	objBuilder << REPO_LABEL_BINARY_BUFFER << (mongo::BSONObj)fileRef;

	auto obj = objBuilder.obj();

	mongo::BSONObjBuilder builder;
	builder.append(REPO_LABEL_BINARY_REFERENCE, obj);
	builder.appendElementsUnique(*this);

	*this = builder.obj();
}

repo::lib::RepoUUID RepoBSON::getUUIDField(const std::string &label) const {
	return hasField(label) ?
		repo::lib::RepoUUID::fromBSONElement(getField(label)) :
		repo::lib::RepoUUID::createUUID();
}

std::vector<repo::lib::RepoUUID> RepoBSON::getUUIDFieldArray(const std::string &label) const {
	std::vector<repo::lib::RepoUUID> results;

	if (hasField(label))
	{
		RepoBSON array = getObjectField(label);

		if (!array.isEmpty())
		{
			std::set<std::string> fields = array.getFieldNames();

			std::set<std::string>::iterator it;
			for (it = fields.begin(); it != fields.end(); ++it)
				results.push_back(array.getUUIDField(*it));
		}
		else
		{
			repoDebug << "getUUIDFieldArray: field " << label << " is an empty bson or wrong type!";
		}
	}

	return results;
}

std::vector<uint8_t> RepoBSON::getBigBinary(
	const std::string &key) const
{
	std::vector<uint8_t> binary;
	const auto &it = bigFiles.find(key);

	if (it != bigFiles.end())
	{
		binary = it->second.second;
	}
	else
	{
		repoError << "External binary not found for key " << key << "! (size of mapping is : " << bigFiles.size() << ")";
	}

	return binary;
}

std::vector<std::pair<std::string, std::string>> RepoBSON::getFileList() const
{
	std::vector<std::pair<std::string, std::string>> fileList;
	if (hasField(REPO_LABEL_OVERSIZED_FILES))
	{
		RepoBSON extRefbson = getObjectField(REPO_LABEL_OVERSIZED_FILES);

		std::set<std::string> fieldNames = extRefbson.getFieldNames();
		for (const auto &name : fieldNames)
		{
			fileList.push_back(std::pair<std::string, std::string>(name, extRefbson.getStringField(name)));
		}
	}

	return fileList;
}

std::pair<repo::core::model::RepoBSON, uint8_t*> RepoBSON::initBinaryBuffer() {
	std::pair<repo::core::model::RepoBSON, uint8_t*> res;
	if (hasField(REPO_LABEL_BINARY_REFERENCE))
	{
		RepoBSON extRefbson = getObjectField(REPO_LABEL_BINARY_REFERENCE);
		res.first = extRefbson.getObjectField(REPO_LABEL_BINARY_BUFFER);

		size_t bufferSize = extRefbson.getObjectField(REPO_LABEL_BINARY_BUFFER).getIntField(REPO_LABEL_BINARY_SIZE);
		binBuffer.resize(bufferSize);
		res.second = binBuffer.data();
	}

	return res;
}

bool RepoBSON::hasLegacyFileReference() const {
	return hasField(REPO_LABEL_OVERSIZED_FILES);
}
bool RepoBSON::hasFileReference() const {
	return hasField(REPO_LABEL_BINARY_REFERENCE);
}

std::vector<float> RepoBSON::getFloatArray(const std::string &label) const
{
	std::vector<float> results;
	if (hasField(label))
	{
		RepoBSON array = getObjectField(label);
		if (!array.isEmpty())
		{
			std::set<std::string> fields = array.getFieldNames();

			// Pre allocate memory to speed up copying
			results.reserve(fields.size());
			for (auto field : fields)
				results.push_back(array.getDoubleField(field));
		}
		else
		{
			repoError << "getFloatArray: field " << label << " is an empty bson or wrong type!";
		}
	}
	return results;
}

std::vector<std::string> RepoBSON::getStringArray(const std::string &label) const
{
	std::vector<std::string> results;
	if (hasField(label))
	{
		std::vector<RepoBSONElement> array = getField(label).Array();
		// Pre allocate memory to speed up copying
		results.reserve(array.size());
		for (auto element : array)
			results.push_back(element.String());
	}
	return results;
}

int64_t RepoBSON::getTimeStampField(const std::string &label) const
{
	int64_t time = -1;

	if (hasField(label))
	{
		auto field = getField(label);
		if (field.type() == ElementType::DATE)
			time = field.date().asInt64();
		else
		{
			repoError << "GetTimeStampField: field " << label << " is not of type Date!";
		}
	}
	return time;
}

std::list<std::pair<std::string, std::string> > RepoBSON::getListStringPairField(
	const std::string &arrLabel,
	const std::string &fstLabel,
	const std::string &sndLabel) const
{
	std::list<std::pair<std::string, std::string> > list;
	mongo::BSONElement arrayElement;
	if (hasField(arrLabel))
	{
		arrayElement = mongo::BSONObj::getField(arrLabel);
	}

	if (!arrayElement.eoo() && arrayElement.type() == mongo::BSONType::Array)
	{
		std::vector<mongo::BSONElement> array = arrayElement.Array();
		for (const auto &element : array)
		{
			if (element.type() == mongo::BSONType::Object)
			{
				mongo::BSONObj obj = element.embeddedObject();
				if (obj.hasField(fstLabel) && obj.hasField(sndLabel))
				{
					std::string field1 = obj.getField(fstLabel).String();
					std::string field2 = obj.getField(sndLabel).String();
					list.push_back(std::make_pair(field1, field2));
				}
			}
		}
	}
	return list;
}

double RepoBSON::getEmbeddedDouble(
	const std::string &embeddedObjName,
	const std::string &fieldName,
	const double &defaultValue) const
{
	double value = defaultValue;
	if (hasEmbeddedField(embeddedObjName, fieldName))
	{
		value = (getObjectField(embeddedObjName)).getDoubleField(fieldName);
	}
	return value;
}

double RepoBSON::getDoubleField(const std::string &label) const {
	double ret = 0.0;
	if (mongo::BSONObj::hasField(label)) {
		auto field = mongo::BSONObj::getField(label);
		if (field.type() == mongo::BSONType::NumberDouble)
			ret = field.Double();
	}

	return ret;
}

bool RepoBSON::hasEmbeddedField(
	const std::string &embeddedObjName,
	const std::string &fieldName) const
{
	bool found = false;
	if (hasField(embeddedObjName))
	{
		found = (getObjectField(embeddedObjName)).hasField(fieldName);
	}
	return found;
}