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

#include <exception>
#include <string>
#include <memory>
#include <variant>
#include <set>

#include <repo/lib/datastructure/repo_container.h>

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				struct ClashDetectionException
				{
					// For simplicitly, we'd like to pass the actual exception object which
					// already has all the information to the report, however the thrown
					// object may go out of scope before we're done with it, so this virtual
					// method allows each exception to copy itself, preserving its data when
					// handled by logic only written to deal with the base class.

					virtual std::shared_ptr<ClashDetectionException> clone() const = 0;
					~ClashDetectionException() = default;
				};

				struct ValidationException : public ClashDetectionException {

				};

				struct MeshBoundsException : public ValidationException
				{
					MeshBoundsException(const repo::lib::Container& container, const repo::lib::RepoUUID& uniqueId);

					repo::lib::Container container;
					repo::lib::RepoUUID uniqueId;

					virtual std::shared_ptr<ClashDetectionException> clone() const override;
				};

				struct TransformBoundsException : public ValidationException
				{
					TransformBoundsException(const repo::lib::Container& container, const repo::lib::RepoUUID& uniqueId);

					repo::lib::Container container;
					repo::lib::RepoUUID uniqueId;

					virtual std::shared_ptr<ClashDetectionException> clone() const override;
				};

				struct OverlappingSetsException : public ValidationException
				{
					OverlappingSetsException(std::set<repo::lib::RepoUUID> overlappingCompositeIds);
					
					std::set<repo::lib::RepoUUID> compositeIds;

					virtual std::shared_ptr<ClashDetectionException> clone() const override;
				};

				struct DuplicateMeshIdsException : public ValidationException {
					DuplicateMeshIdsException(const repo::lib::RepoUUID& uniqueId);

					repo::lib::RepoUUID uniqueId;

					virtual std::shared_ptr<ClashDetectionException> clone() const override;
				};
			}
		}
	}
}