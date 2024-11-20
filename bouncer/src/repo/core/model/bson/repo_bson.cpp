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
#include <unordered_map>
#include "repo/lib/repo_exception.h"
#include "repo/core/model/bson/repo_bson_builder.h"

#include <bsoncxx/json.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/exception/exception.hpp>

using namespace repo::core::model;

RepoBSON::RepoBSON(const RepoBSON &obj,
	const BinMapping &binMapping) :
	bsoncxx::document::value(obj.view()),
	bigFiles(binMapping) {

	auto existingFiles = obj.getFilesMapping();
	for (const auto& pair : existingFiles) {
		if (bigFiles.find(pair.first) == bigFiles.end()) {
			bigFiles[pair.first] = pair.second;
		}
	}
}

RepoBSON::RepoBSON(
	const bsoncxx::document::view &obj,
	const BinMapping &binMapping)
	: bsoncxx::document::value(obj),
	bigFiles(binMapping)
{
}

RepoBSON::RepoBSON()
	: bsoncxx::document::value(bsoncxx::builder::basic::make_document())
{
}

bool RepoBSON::hasField(const std::string& label) const
{
	return bsoncxx::document::value::find(label) != bsoncxx::document::value::end();
}

RepoBSONElement RepoBSON::getField(const std::string& label) const
{
	auto value = bsoncxx::document::value::find(label);
	if (value != bsoncxx::document::value::end())
	{
		return RepoBSONElement(*value);
	}
	else
	{
		throw repo::lib::RepoFieldNotFoundException(label);
	}
}

bool RepoBSON::getBoolField(const std::string& label) const
{
	try
	{
		return getField(label).Bool();
	}
	catch (bsoncxx::exception)
	{
		throw repo::lib::RepoFieldTypeException(label);
	}
}

std::string RepoBSON::getStringField(const std::string& label) const
{
	try
	{
		return getField(label).String();
	}
	catch (bsoncxx::exception)
	{
		throw repo::lib::RepoFieldTypeException(label);
	}
}

RepoBSON RepoBSON::getObjectField(const std::string& label) const
{
	try
	{
		return getField(label).Object();
	}
	catch (bsoncxx::exception)
	{
		throw repo::lib::RepoFieldTypeException(label);
	}
}

std::vector<repo::lib::RepoVector3D> RepoBSON::getBounds3D(const std::string& label)
{
	auto field = getObjectField(label);
	return std::vector< lib::RepoVector3D>({
		lib::RepoVector3D(field.getFloatVectorField("0")),
		lib::RepoVector3D(field.getFloatVectorField("1")),
	});
}

bool RepoBSON::isEmpty() const
{
	return bsoncxx::document::value::empty() && !bigFiles.size();
}

int RepoBSON::getIntField(const std::string& label) const
{
	try
	{
		return getField(label).Int();
	}
	catch (bsoncxx::exception)
	{
		throw repo::lib::RepoFieldTypeException(label);
	}
}

std::set<std::string> RepoBSON::getFieldNames() const
{
	std::set<std::string> fieldNames;
	for (auto& f : *this) {
		std::string n(f.key().data(), f.key().size());
		fieldNames.insert(n);
	}
	return fieldNames;
}

uint64_t RepoBSON::objsize() const
{
	return bsoncxx::document::value::length();
}

std::string RepoBSON::toString() const
{
	return bsoncxx::to_json(this->view());
}

bool RepoBSON::operator==(const RepoBSON other) const
{
	return this->view() == other.view() && bigFiles == other.bigFiles;
}

bool RepoBSON::operator!=(const RepoBSON other) const
{
	return this->view() != other.view() || bigFiles != other.bigFiles;
}

RepoBSON& RepoBSON::operator=(RepoBSON otherCopy)
{
	bsoncxx::document::value::reset(otherCopy.view());
	bigFiles = otherCopy.bigFiles;
	return *this;
}

void RepoBSON::swap(RepoBSON otherCopy)
{
	*this = otherCopy;
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

std::pair<repo::core::model::RepoBSON, std::vector<uint8_t>> RepoBSON::getBinariesAsBuffer() const
{
	std::pair<repo::core::model::RepoBSON, std::vector<uint8_t>> res;
	if (bigFiles.size()) {
		std::vector<uint8_t> &buffer = res.second;

		RepoBSONBuilder elemsBuilder;
		for (const auto &entry : bigFiles) {
			RepoBSONBuilder entryBuilder;

			entryBuilder.append(REPO_LABEL_BINARY_START, (int32_t)buffer.size());

			buffer.insert(buffer.end(), entry.second.begin(), entry.second.end());
			entryBuilder.append(REPO_LABEL_BINARY_SIZE, (int32_t)entry.second.size());

			elemsBuilder.append(entry.first, entryBuilder.obj());
		}

		res.first = elemsBuilder.obj();
	}

	return res;
}

void RepoBSON::replaceBinaryWithReference(
	const repo::core::model::RepoBSON &fileRef,
	const repo::core::model::RepoBSON &elemRef
)
{
	RepoBSONBuilder objBuilder;
	objBuilder.append(REPO_LABEL_BINARY_ELEMENTS, elemRef);
	objBuilder.append(REPO_LABEL_BINARY_BUFFER, fileRef);

	auto obj = objBuilder.obj();

	RepoBSONBuilder builder;
	builder.append(REPO_LABEL_BINARY_REFERENCE, obj);
	builder.appendElementsUnique(*this);

	*this = builder.obj();
}

repo::lib::RepoUUID RepoBSON::getUUIDField(const std::string &label) const
{
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

std::vector<float> RepoBSON::getFloatVectorField(const std::string& label) const
{
	std::vector<float> results;
	auto a = getObjectField(label);
	for (auto& f : a)
	{
		try {
			results.push_back(f.get_double());
		}
		catch (bsoncxx::exception)
		{
			throw repo::lib::RepoFieldTypeException(label); // Will get a bsoncxx exception in case of a cast error - otherwise the exception should bubble up
		}
	}
	return results;
}

repo::lib::RepoVector3D RepoBSON::getVector3DField(const std::string& label) const
{
	auto f = getObjectField(label);
	repo::lib::RepoVector3D v;
	v.x = f.getDoubleField("x");
	v.y = f.getDoubleField("y");
	v.z = f.getDoubleField("z");
	return v;
}

repo::lib::RepoMatrix RepoBSON::getMatrixField(const std::string& label) const
{
	std::vector<RepoBSONElement> rows;
	std::vector<RepoBSONElement> cols;

	std::vector<float> transformationMatrix;

	uint32_t rowInd = 0, colInd = 0;
	transformationMatrix.resize(16);
	float* transArr = &transformationMatrix.at(0);

	// matrix is stored as array of arrays, row first

	auto matrixObj = getField(label).Object();



	/*
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
	*/

	return repo::lib::RepoMatrix(transformationMatrix);
}

std::vector<double> RepoBSON::getDoubleVectorField(const std::string& label) const
{
	std::vector<double> results;
	auto a = getObjectField(label);
	for (auto& f : a)
	{
		try {
			results.push_back(f.get_double());
		}
		catch (bsoncxx::exception)
		{
			throw repo::lib::RepoFieldTypeException(label); // Will get a bsoncxx exception in case of a cast error - otherwise the exception should bubble up
		}
	}
	return results;
}

std::vector<repo::lib::RepoUUID> RepoBSON::getUUIDFieldArray(const std::string &label) const
{
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
	const auto& it = bigFiles.find(field);
	if (it != bigFiles.end())
	{
		return it->second;
	}
	else
	{
		if (hasField(field))
		{
			throw repo::lib::RepoFieldTypeException(field);
		}
		else
		{
			throw repo::lib::RepoFieldNotFoundException(field);
		}
	}
}

repo::core::model::RepoBSON RepoBSON::getBinaryReference() const
{
	repo::core::model::RepoBSON res;
	if (hasField(REPO_LABEL_BINARY_REFERENCE))
	{
		RepoBSON extRefbson = getObjectField(REPO_LABEL_BINARY_REFERENCE);
		return extRefbson.getObjectField(REPO_LABEL_BINARY_BUFFER);
	}

	return res;
}

void RepoBSON::initBinaryBuffer(const std::vector<uint8_t> &buffer)
{
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
	return bigFiles.find(label) != bigFiles.end();
}

bool RepoBSON::hasFileReference() const
{
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
			for (auto& f : array)
			{
				try {
					results.push_back(f.get_double());
				}
				catch (bsoncxx::exception)
				{
					throw repo::lib::RepoFieldTypeException(label); // Will get a bsoncxx exception in case of a cast error - otherwise the exception should bubble up
				}
			}
		}
	}
	return results;
}

std::vector<std::string> RepoBSON::getStringArray(const std::string &label) const
{
	std::vector<std::string> results;
	if (hasField(label))
	{
		RepoBSON array = getObjectField(label);
		for (const auto& f : array)
		{
			try {
				auto view = f.get_string().value;
				results.push_back(std::string(view.data(), view.size()));
			}
			catch (bsoncxx::exception)
			{
				throw repo::lib::RepoFieldTypeException(label); // This version of the driver doesn't have specific exceptions so we guess based on the scope of the try block this is what it is
			}
		}
	}
	return results;
}

time_t RepoBSON::getTimeStampField(const std::string &label) const
{
	try{
		return getField(label).TimeT();
	}
	catch(bsoncxx::exception)
	{
		throw repo::lib::RepoFieldTypeException(label);
	}
}

double RepoBSON::getDoubleField(const std::string &label) const
{
	try
	{
		return getField(label).Double();
	}
	catch (bsoncxx::exception)
	{
		throw repo::lib::RepoFieldTypeException(label);
	}
}

long long RepoBSON::getLongField(const std::string& label) const
{
	try
	{
		auto f = getField(label);
		if (f.type() == repo::core::model::ElementType::INT) { // We can convert integers to longs OK
			return f.Int();
		}
		return getField(label).Long();
	}
	catch (bsoncxx::exception)
	{
		throw repo::lib::RepoFieldTypeException(label);
	}
}