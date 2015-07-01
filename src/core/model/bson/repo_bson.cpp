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
//
////------------------------------------------------------------------------------
//
//int repo::core::model::bson::RepoBSON::addFields(mongo::BSONObj &obj)
//{
//	int success = 0;
//	if (isEmpty() && !isOwned())
//	{
//		std::set<std::string> fields;
//		obj.getFieldNames(fields);
//		success = mongo::BSONObj::addFields(obj, fields);
//	}
//	return success;
//}
//
//repo::core::model::bson::RepoBSON repo::core::model::bson::RepoBSON::drop(const std::string &collection) const
//{
//	mongo::BSONObjBuilder builder;
//
//	if (hasField(REPO_LABEL_ID))
//	{
//		//----------------------------------------------------------------------
//		// Delete
//		builder << REPO_COMMAND_DELETE << collection;
//
//		//----------------------------------------------------------------------
//		// Deletes
//		mongo::BSONObjBuilder deletesBuilder;
//		deletesBuilder << REPO_COMMAND_Q << BSON(REPO_LABEL_ID << this->getField(REPO_LABEL_ID));
//		deletesBuilder << REPO_COMMAND_LIMIT << 1;
//		builder << REPO_COMMAND_DELETES << BSON_ARRAY(deletesBuilder.obj());
//	}
//	return builder.obj();
//}
//
//std::list<std::pair<std::string, std::string> > repo::core::model::bson::RepoBSON::getArrayStringPairs(
//	const mongo::BSONElement &arrayElement,
//	const std::string &fstLabel,
//	const std::string &sndLabel)
//{
//	std::list<std::pair<std::string, std::string> > list;
//	if (!arrayElement.eoo())
//	{
//		std::vector<mongo::BSONElement> array = arrayElement.Array();
//		for (unsigned int i = 0; i < array.size(); ++i)
//		{
//			if (array[i].type() == mongo::BSONType::Object)
//			{
//				mongo::BSONObj obj = array[i].embeddedObject();
//				if (obj.hasField(fstLabel) && obj.hasField(sndLabel))
//				{
//					std::string field1 = obj.getField(fstLabel).String();
//					std::string field2 = obj.getField(sndLabel).String();
//					list.push_back(std::make_pair(field1, field2));
//				}
//			}
//		}
//	}
//	return list;
//}
//
//mongo::BSONElement repo::core::model::bson::RepoBSON::getEmbeddedElement(
//	const std::string &fstLevelLabel,
//	const std::string &sndLevelLabel) const
//{
//	mongo::BSONElement element;
//	if (this->hasField(fstLevelLabel))
//	{
//		mongo::BSONObj embeddedData = this->getObjectField(fstLevelLabel);
//		if (embeddedData.hasField(sndLevelLabel))
//			element = embeddedData.getField(sndLevelLabel);
//	}
//	return element;
//}
//
//mongo::BSONArray repo::core::model::bson::RepoBSON::toArray(
//	const std::list<std::pair<std::string, std::string> > &list,
//	const std::string &fstLabel,
//	const std::string &sndLabel)
//{
//	mongo::BSONArrayBuilder builder;
//	std::list<std::pair<std::string, std::string> >::const_iterator i;
//	for (i = list.begin(); i != list.end(); ++i)
//	{
//		mongo::BSONObjBuilder innerBuilder;
//		innerBuilder << fstLabel << i->first;
//		innerBuilder << sndLabel << i->second;
//		builder.append(innerBuilder.obj());
//	}
//	return builder.arr();
//}
