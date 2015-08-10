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

using namespace repo::core::model::bson;

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