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


using namespace repo::core::model;

RepoBSON::RepoBSON(const mongo::BSONObj &obj,
	const std::unordered_map<std::string, std::vector<uint8_t>> &binMapping)
	: mongo::BSONObj(obj),
	bigFiles(binMapping)
{
	if (!obj.hasField(REPO_LABEL_OVERSIZED_FILES))
	{
		//Append oversize file references into the bson
		std::vector<std::string> fnames;
		boost::copy(
			bigFiles | boost::adaptors::map_keys,
			std::back_inserter(fnames));

		mongo::BSONObjBuilder builder, arrbuilder;
		if (fnames.size() > 0)
		{
			for (int i = 0; i < fnames.size(); ++i)
			{
				arrbuilder << std::to_string(i) << fnames[i];
			}

			builder.appendArray(REPO_LABEL_OVERSIZED_FILES, arrbuilder.obj());
			builder.appendElementsUnique(obj);

			this->swap(builder.obj());
			bigFiles = binMapping;
		}
	}


}

repoUUID RepoBSON::getUUIDField(const std::string &label) const{
	repoUUID uuid;
	const mongo::BSONElement bse = getField(label);
	if (bse.binDataType() == mongo::bdtUUID ||
		bse.binDataType() == mongo::newUUID)
	{
		int len = static_cast<int>(bse.size() * sizeof(boost::uint8_t));
		const char *binData = bse.binData(len);
		memcpy(uuid.data, binData, len);
	}
	else
		uuid = boost::uuids::random_generator()();  // failsafe
	return uuid;
}


std::vector<repoUUID> RepoBSON::getUUIDFieldArray(const std::string &label) const{
	std::vector<repoUUID> results;

	if (hasField(label))
	{
		RepoBSON array = RepoBSON(getField(label).embeddedObject());

		std::set<std::string> fields;
		array.getFieldNames(fields);

		std::set<std::string>::iterator it;
		for (it = fields.begin(); it != fields.end(); ++it)
			results.push_back(array.getUUIDField(*it));

	}
	return results;
}

std::vector<uint8_t> RepoBSON::getBigBinary(
	const std::string &key) const
{
	std::vector<uint8_t> binary;
	const auto &it = bigFiles.find(key);

	if (it != bigFiles.end())
		binary = it->second;
	else
	{
		repoError << "External binary not found for key " << key << "! (size of mapping is : " << bigFiles.size() << ")";
	}

	return binary;
}

std::vector<std::string> RepoBSON::getFileList() const
{
	std::vector<std::string> fileList;
	if (hasField(REPO_LABEL_OVERSIZED_FILES))
	{
		RepoBSON arraybson = getObjectField(REPO_LABEL_OVERSIZED_FILES);

		std::set<std::string> fields;
		arraybson.getFieldNames(fields);

		for (const auto &field : fields)
		{
			fileList.push_back(arraybson.getStringField(field));
		}
	}

	return fileList;
}

std::vector<float> RepoBSON::getFloatArray(const std::string &label) const
{
	std::vector<float> results;

	if (hasField(label))
	{
		RepoBSON array = RepoBSON(getField(label).embeddedObject());

		std::set<std::string> fields;
		array.getFieldNames(fields);

		for (auto field: fields)
			results.push_back(array.getField(field).numberDouble());

	}
	return results;
}

int64_t RepoBSON::getTimeStampField(const std::string &label) const
{
	int64_t time;

	if (hasField(label))
	{
		time = getField(label).date().asInt64();


	}
	return time;
}


std::list<std::pair<std::string, std::string> > RepoBSON::getListStringPairField (
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
