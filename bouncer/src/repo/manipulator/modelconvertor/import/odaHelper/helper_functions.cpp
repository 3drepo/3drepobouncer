/**
*  Copyright (C) 2018 3D Repo Ltd
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

#include <boost/locale/encoding_utf.hpp>
#include "helper_functions.h"

using namespace boost::locale::conv;

std::string repo::manipulator::modelconvertor::odaHelper::convertToStdString(const OdString &value)
{
	std::wstring wstr((wchar_t*)value.c_str());
	std::string str = utf_to_utf<char>(wstr.c_str(), wstr.c_str() + wstr.size());
	return str;
}