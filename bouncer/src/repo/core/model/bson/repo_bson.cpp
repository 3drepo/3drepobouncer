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

RepoBSON::RepoBSON(
	const mongo::BSONObj &obj,
	const std::unordered_map<std::string, std::pair<std::string, std::vector<uint8_t>>> &binMapping)
	: mongo::BSONObj(obj),
	bigFiles(binMapping)
{
	std::vector<std::pair<std::string, std::string>> existingFiles;

	if (bigFiles.size() > 0)
	{
		mongo::BSONObjBuilder builder, arrbuilder;

		for (const auto & pair : bigFiles)
		{
			//append field name :file name
			arrbuilder << pair.first << pair.second.first;
		}

		if (obj.hasField(REPO_LABEL_OVERSIZED_FILES))
		{
			arrbuilder.appendElementsUnique(obj.getObjectField(REPO_LABEL_OVERSIZED_FILES));
		}

		builder.append(REPO_LABEL_OVERSIZED_FILES, arrbuilder.obj());
		builder.appendElementsUnique(obj);

		*this = builder.obj();
		bigFiles = binMapping;
	}
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
	std::set<std::string> fields;
	std::unordered_map< std::string, std::pair<std::string, std::vector<uint8_t>>> rawFiles(bigFiles.begin(), bigFiles.end());
	std::string uniqueIDStr = hasField(REPO_LABEL_ID) ? getUUIDField(REPO_LABEL_ID).toString() : repo::lib::RepoUUID::createUUID().toString();

	getFieldNames(fields);

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

repo::lib::RepoUUID RepoBSON::getUUIDField(const std::string &label) const{

	
	if (hasField(label))
	{
		const mongo::BSONElement bse = getField(label);
		return repo::lib::RepoUUID::fromBSONElement(bse);		
	}	
	else
	{
		return repo::lib::RepoUUID::createUUID();
	}

}

std::vector<repo::lib::RepoUUID> RepoBSON::getUUIDFieldArray(const std::string &label) const{
	std::vector<repo::lib::RepoUUID> results;

	if (hasField(label))
	{
		RepoBSON array = getObjectField(label);

		if (!array.isEmpty())
		{
			std::set<std::string> fields;
			array.getFieldNames(fields);

			std::set<std::string>::iterator it;
			for (it = fields.begin(); it != fields.end(); ++it)
				results.push_back(array.getUUIDField(*it));
		}
		else
		{
			repoError << "getUUIDFieldArray: field " << label << " is an empty bson or wrong type!";
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

		std::set<std::string> fieldNames;
		extRefbson.getFieldNames(fieldNames);
		for (const auto &name : fieldNames)
		{
			fileList.push_back(std::pair<std::string, std::string>(name, extRefbson.getStringField(name)));
		}
	}

	return fileList;
}

std::vector<float> RepoBSON::getFloatArray(const std::string &label) const
{
	std::vector<float> results;
	if (hasField(label))
	{
		RepoBSON array = getObjectField(label);
		if (!array.isEmpty())
		{
			std::set<std::string> fields;
			array.getFieldNames(fields);

			// Pre allocate memory to speed up copying
			results.reserve(fields.size());
			for (auto field : fields)
				results.push_back(array.getField(field).numberDouble());
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
		else if (!field.isNull())
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
		arrayElement = getField(arrLabel);
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
		value = (getObjectField(embeddedObjName)).getField(fieldName).numberDouble();
	}
	return value;
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