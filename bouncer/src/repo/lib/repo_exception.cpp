/**
*  Copyright (C) 2024 3D Repo Ltd
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

#pragma once

#include "repo_exception.h"
#include <sstream>

using namespace repo::lib;

void printNestedExceptions(const std::exception& e, std::stringstream& stream, int indent = 0)
{
	stream << std::string(indent, '\t') << e.what() << std::endl;
	try {
		std::rethrow_if_nested(e);
	}
	catch (const std::exception& nested)
	{
		printNestedExceptions(nested, stream, indent + 1);
	}
}

RepoException::RepoException(const std::string& msg)
	: errMsg(msg), errorCode(REPOERR_UNKNOWN_ERR) 
{
}

char const* RepoException::what() const throw()
{
	return errMsg.c_str();
}

int RepoException::repoCode() const
{
	return errorCode;
}

std::string RepoException::printFull() const
{
	std::stringstream stream;
	printNestedExceptions(*this, stream);
	return stream.str();
}

RepoFieldNotFoundException::RepoFieldNotFoundException(const std::string& fieldName)
	: RepoBSONException("BSON does not have the field: " + fieldName) 
{
}

RepoFieldTypeException::RepoFieldTypeException(const std::string& fieldName)
	: RepoBSONException("BSON field " + fieldName + " attempting to be read as a different type") 
{
}

RepoBSONException::RepoBSONException(const std::string& msg)
	: RepoException(msg) 
{
}

RepoInvalidLicenseException::RepoInvalidLicenseException(const std::string& msg)
	: RepoException(msg)
{
	errorCode = ERRCODE_REPO_LICENCE_INVALID;
};