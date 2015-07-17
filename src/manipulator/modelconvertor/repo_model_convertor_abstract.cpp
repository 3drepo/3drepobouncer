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

/**
* Allows Import/Export functionality into/output Repo world using ASSIMP
*/


#include "repo_model_convertor_abstract.h"
#include <boost/filesystem.hpp>

using namespace repo::manipulator::modelconvertor;

AbstractModelConvertor::AbstractModelConvertor()
{
	settings = new ModelConvertorConfig();
}

AbstractModelConvertor::AbstractModelConvertor(ModelConvertorConfig *settings) :
settings(settings)
{
	if (!settings)
	{
		//settings is null, used default
		settings = new ModelConvertorConfig();
	}
}

AbstractModelConvertor::~AbstractModelConvertor()
{
	if (settings)
		delete settings;
}

std::string AbstractModelConvertor::getDirPath(std::string fullPath){
	boost::filesystem::path p{ fullPath };
	return p.parent_path().string();
}

std::string AbstractModelConvertor::getFileName(std::string fullPath){
	boost::filesystem::path p{ fullPath };
	return p.filename().string();
}
