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

std::shared_ptr<ClashDetectionException> MeshBoundsException::clone() const {
	return std::make_shared<MeshBoundsException>(*this);
}

TransformBoundsException::TransformBoundsException(const repo::lib::Container& container, const repo::lib::RepoUUID& uniqueId)
	: container(container), uniqueId(uniqueId)
{
}

std::shared_ptr<ClashDetectionException> TransformBoundsException::clone() const {
	return std::make_shared<TransformBoundsException>(*this);
}

OverlappingSetsException::OverlappingSetsException(std::set<repo::lib::RepoUUID> overlappingCompositeIds)
	:compositeIds(overlappingCompositeIds)
{
}

std::shared_ptr<ClashDetectionException> OverlappingSetsException::clone() const {
	return std::make_shared<OverlappingSetsException>(*this);
}

DuplicateMeshIdsException::DuplicateMeshIdsException(const repo::lib::RepoUUID& uniqueId)
	: uniqueId(uniqueId)
{
}

std::shared_ptr<ClashDetectionException> DuplicateMeshIdsException::clone() const {
	return std::make_shared<DuplicateMeshIdsException>(*this);
}
