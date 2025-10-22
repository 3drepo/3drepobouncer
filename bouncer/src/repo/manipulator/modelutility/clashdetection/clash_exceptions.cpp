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

#include "clash_exceptions.h"

#include <repo/manipulator/modelutility/repo_clash_detection_config.h>

using namespace repo::manipulator::modelutility::clash;

MeshBoundsException::MeshBoundsException(const repo::lib::Container& container, const repo::lib::RepoUUID& uniqueId)
	: container(container), uniqueId(uniqueId)
{
}

const char* MeshBoundsException::what() const noexcept {
	return "Mesh has exceeded the maximum dimensions. See exception object members for identity information.";
}

TransformBoundsException::TransformBoundsException(const repo::lib::Container& container, const repo::lib::RepoUUID& uniqueId)
	: container(container), uniqueId(uniqueId)
{

}
const char* TransformBoundsException::what() const noexcept {
	return "Mesh has a World transform that exceeds the maximum dimensions in either scale or translation. See exception object members for identity information.";
}