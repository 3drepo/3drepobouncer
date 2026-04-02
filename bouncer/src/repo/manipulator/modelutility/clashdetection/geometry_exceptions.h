/**
*  Copyright (C) 2025 3D Repo Ltd
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

namespace geometry
{
	/*
	* An exception thrown to indicate an unrecoverable logic error during a
	* geometry test. Such exceptions are thrown when the process state itself is
	* fine, but the test cannot proceed & provide sensible return values. Whatever
	* algorithm is relying on the result should abort.
	*/
	struct GeometryTestException : public std::runtime_error
	{
		GeometryTestException(const std::string& message)
			: std::runtime_error(message)
		{
		}
	};
}
