/**
*  Copyright (C) 2019 3D Repo Ltd
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

#include <boost/filesystem.hpp>
#include <string>
namespace repo {
	namespace lib {
		static bool doesFileExist(const boost::filesystem::path& inputPath)
		{
			return boost::filesystem::exists(inputPath) && boost::filesystem::is_regular_file(inputPath);
		}

		static bool doesDirExist(const boost::filesystem::path& inputPath)
		{
			return boost::filesystem::exists(inputPath) && boost::filesystem::is_directory(inputPath);
		}

		static std::string getExtension(const boost::filesystem::path &inputPath)
		{
			return inputPath.extension().string();
		}

		static void toLower(std::string &str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
		}

		static void toUpper(std::string &str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
		}

		static std::string getEnvString(std::string const & envVarName)
		{
			char* value = getenv(envVarName.c_str());
			return (value && strlen(value) > 0) ? value : "";
		}
	}
}