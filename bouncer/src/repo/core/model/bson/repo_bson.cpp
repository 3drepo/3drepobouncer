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
#include <mongo/bson/bson.h>

#include "repo/lib/repo_exception.h"

using namespace repo::core::model;

RepoBSON::RepoBSON(const RepoBSON &obj,
	const BinMapping &binMapping) :
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
	const BinMapping &binMapping)
	: mongo::BSONObj(obj),
	bigFiles(binMapping)
{
}

RepoBSON::RepoBSON()
	: mongo::BSONObj()
{
}

RepoBSON::RepoBSON(mongo::BSONObjBuilder& builder)
	: mongo::BSONObj(builder.obj())
{
}

RepoBSON::RepoBSON(const std::vector<char>& rawData)
	: mongo::BSONObj(rawData.data())
{
}

bool RepoBSON::hasField(const std::string& label) const {
	return mongo::BSONObj::hasField(label);
}

int RepoBSON::nFields() const {
	return mongo::BSONObj::nFields();
}

RepoBSONElement RepoBSON::getField(const std::string& label) const
{
	return RepoBSONElement(mongo::BSONObj::getField(label));
}

bool RepoBSON::getBoolField(const std::string& label) const
{
	return mongo::BSONObj::getBoolField(label);
}

std::string RepoBSON::getStringField(const std::string& label) const {
	if (hasField(label))
	{
		auto f = getField(label);
		if (f.type() == ElementType::STRING)
		{
			return f.String();
		}
		else
		{
			throw repo::lib::RepoFieldTypeException(label);
		}
	}
	throw repo::lib::RepoFieldNotFoundException(label);
}

RepoBSON RepoBSON::getObjectField(const std::string& label) const {
	if (hasField(label))
	{
		return mongo::BSONObj::getObjectField(label);
	}
	else
	{
		throw repo::lib::RepoFieldNotFoundException(label);
	}
}

std::vector<repo::lib::RepoVector3D> RepoBSON::getBounds3D(const std::string& label) {
	auto field = getObjectField(label);
	return std::vector< lib::RepoVector3D>({
		lib::RepoVector3D(field.getFloatVectorField("0")),
		lib::RepoVector3D(field.getFloatVectorField("1")),
		});
}

bool RepoBSON::isEmpty() const {
	return mongo::BSONObj::isEmpty();
}

int RepoBSON::getIntField(const std::string& label) const {
	return mongo::BSONObj::getIntField(label);
}

std::set<std::string> RepoBSON::getFieldNames() const {
	std::set<std::string> fieldNames;
	mongo::BSONObj::getFieldNames(fieldNames);
	return fieldNames;
}

uint64_t RepoBSON::objsize() const {
	return mongo::BSONObj::objsize();
}

RepoBSON RepoBSON::removeField(const std::string& label) const {
	return mongo::BSONObj::removeField(label);
}

std::string RepoBSON::toString() const {
	return mongo::BSONObj::toString();
}

bool RepoBSON::operator==(const RepoBSON other) const {
	return mongo::BSONObj::operator==((mongo::BSONObj)other);
}

bool RepoBSON::operator!=(const RepoBSON other) const {
	return mongo::BSONObj::operator!=((mongo::BSONObj)other);
}


RepoBSON& RepoBSON::operator=(RepoBSON otherCopy) {
	swap(otherCopy);
	return *this;
}

void RepoBSON::swap(RepoBSON otherCopy)
{
	mongo::BSONObj::swap(otherCopy);
	bigFiles = otherCopy.bigFiles;
}

std::vector<std::string> RepoBSON::getFileList(const std::string& label) const
{
	std::vector<std::string> fileList;
	RepoBSON arraybson = getObjectField(label);
	std::set<std::string> fields = arraybson.getFieldNames();
	for (const auto& field : fields)
	{
		fileList.push_back(arraybson.getStringField(field));
	}
	return fileList;
}

std::pair<repo::core::model::RepoBSON, std::vector<uint8_t>> RepoBSON::getBinariesAsBuffer() const {
	std::pair<repo::core::model::RepoBSON, std::vector<uint8_t>> res;
	if (bigFiles.size()) {
		std::vector<uint8_t> &buffer = res.second;
		mongo::BSONObjBuilder elemsBuilder;

		for (const auto &entry : bigFiles) {
			mongo::BSONObjBuilder entryBuilder;

			entryBuilder << REPO_LABEL_BINARY_START << (unsigned int)buffer.size();
			buffer.insert(buffer.end(), entry.second.begin(), entry.second.end());
			entryBuilder << REPO_LABEL_BINARY_SIZE << (unsigned int)entry.second.size();

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
	if (hasField(label))
	{
		auto f = getField(label);
		if (f.type() == repo::core::model::ElementType::UUID)
		{
			boost::uuids::uuid id;
			int len = static_cast<int>(f.size() * sizeof(uint8_t));
			const char* binData = f.binData(len);
			memcpy(id.data, binData, len);
			return repo::lib::RepoUUID(id);
		}
		else
		{
			throw repo::lib::RepoFieldTypeException(label);
		}
	}
	else
	{
		throw repo::lib::RepoFieldNotFoundException(label);
	}
}

std::vector<float> RepoBSON::getFloatVectorField(const std::string& label) const {
	std::vector<float> results;
	if (hasField(label))
	{
		RepoBSON array = getObjectField(label);
		if (!array.isEmpty())
		{
			std::set<std::string> fields = array.getFieldNames();
			results.resize(fields.size());
			std::set<std::string>::iterator it;
			for (it = fields.begin(); it != fields.end(); ++it)
				results.push_back(array.getDoubleField(*it));
		}
		else
		{
			repoDebug << "getFloatVectorField: field " << label << " is an empty bson or wrong type!";
		}
	}
	return results;
}

repo::lib::RepoVector3D RepoBSON::getVector3DField(const std::string& label) const {
	auto f = getObjectField(label);
	repo::lib::RepoVector3D v;
	v.x = f.getDoubleField("x");
	v.y = f.getDoubleField("y");
	v.z = f.getDoubleField("z");
	return v;
}

repo::lib::RepoMatrix RepoBSON::getMatrixField(const std::string& label) const {

	std::vector<mongo::BSONElement> rows;
	std::vector<mongo::BSONElement> cols;

	std::vector<float> transformationMatrix;

	uint32_t rowInd = 0, colInd = 0;
	transformationMatrix.resize(16);
	float* transArr = &transformationMatrix.at(0);

	// matrix is stored as array of arrays, row first

	auto matrixObj = getField(label).embeddedObject();

	matrixObj.elems(rows);
	for (size_t rowInd = 0; rowInd < 4; rowInd++)
	{
		cols.clear();
		rows[rowInd].embeddedObject().elems(cols);
		for (size_t colInd = 0; colInd < 4; colInd++)
		{
			uint32_t index = rowInd * 4 + colInd;
			transArr[index] = cols[colInd].number();
		}
	}

	return repo::lib::RepoMatrix(transformationMatrix);
}

std::vector<double> RepoBSON::getDoubleVectorField(const std::string& label) const {
	std::vector<double> results;
	if (hasField(label))
	{
		RepoBSON array = getObjectField(label);
		if (!array.isEmpty())
		{
			std::set<std::string> fields = array.getFieldNames();
			std::set<std::string>::iterator it;
			for (it = fields.begin(); it != fields.end(); ++it)
				results.push_back(array.getDoubleField(*it));
		}
		else
		{
			repoDebug << "getVectorAsStdDouble: field " << label << " is an empty bson or wrong type!";
		}
	}
	return results;
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
				results.push_back(array.getUUIDField(*it)); // If the item is the wrong type an exception will be thrown to the caller
		}
	}

	return results;
}

const std::vector<uint8_t>& RepoBSON::getBinary(const std::string& field) const
{
	if (!hasField(field) || getField(field).type() == ElementType::STRING)
	{
		const auto& it = bigFiles.find(field);
		if (it != bigFiles.end())
		{
			return it->second;
		}
	}
	else {
		RepoBSONElement bse = getField(field);
		if (bse.type() == ElementType::BINARY && bse.binDataType() == mongo::BinDataGeneral)
		{
			bse.value();
			int length;
			auto binData = (const uint8_t*)bse.binData(length);
			if (length > 0)
			{
				return std::vector<uint8_t>(binData, binData + length);
			}
			else
			{
				throw repo::lib::RepoFieldTypeException(field + "; has a length of zero bytes");
			}
		}
		else
		{
			throw repo::lib::RepoFieldTypeException(field);
		}
	}

	throw repo::lib::RepoFieldNotFoundException(field);
}

repo::core::model::RepoBSON RepoBSON::getBinaryReference() const {
	repo::core::model::RepoBSON res;
	if (hasField(REPO_LABEL_BINARY_REFERENCE))
	{
		RepoBSON extRefbson = getObjectField(REPO_LABEL_BINARY_REFERENCE);
		return extRefbson.getObjectField(REPO_LABEL_BINARY_BUFFER);
	}

	return res;
}

void RepoBSON::initBinaryBuffer(const std::vector<uint8_t> &buffer) {
	if (hasField(REPO_LABEL_BINARY_REFERENCE))
	{
		RepoBSON extRefbson = getObjectField(REPO_LABEL_BINARY_REFERENCE);

		auto elemRefs = extRefbson.getObjectField(REPO_LABEL_BINARY_ELEMENTS);

		for (const auto &elem : elemRefs.getFieldNames()) {
			auto elemRefBson = elemRefs.getObjectField(elem);
			size_t start = elemRefBson.getIntField(REPO_LABEL_BINARY_START);
			size_t size = elemRefBson.getIntField(REPO_LABEL_BINARY_SIZE);

			bigFiles[elem] = std::vector<uint8_t>(buffer.begin() + start, buffer.begin() + start + size);
		}
	}
}

bool RepoBSON::hasBinField(const std::string &label) const
{
	return hasField(label) || bigFiles.find(label) != bigFiles.end();
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

time_t RepoBSON::getTimeStampField(const std::string &label) const
{
	time_t time = 0;

	if (hasField(label))
	{
		auto field = getField(label);
		if (field.type() == ElementType::DATE)
			time = field.TimeT();
		else
		{
			repoError << "GetTimeStampField: field " << label << " is not of type Date!";
		}
	}
	return time;
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

long long RepoBSON::getLongField(const std::string& label) const
{
	auto field = mongo::BSONObj::getField(label);
	return field.Long();
}