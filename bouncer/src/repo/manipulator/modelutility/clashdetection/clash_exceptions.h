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

#include <repo/manipulator/modelutility/repo_clash_detection_config_fwd.h>
#include <repo/lib/datastructure/repo_container.h>

namespace repo {
	namespace manipulator {
		namespace modelutility {
			namespace clash {

				struct ClashDetectionException : public std::exception 
				{
				};

				struct ValidationException : public ClashDetectionException {
					const char* what() const noexcept override {
						return "Input data (such as the scene graph) failed validation during clash detection. Check the exception subclass for details.";
					}
				};

				struct MeshBoundsException : public ValidationException
				{
					MeshBoundsException(const repo::lib::Container& container, const repo::lib::RepoUUID& uniqueId);

					const char* what() const noexcept override;

					repo::lib::Container container;
					repo::lib::RepoUUID uniqueId;
				};

				struct TransformBoundsException : public ValidationException
				{
					TransformBoundsException(const repo::lib::Container& container, const repo::lib::RepoUUID& uniqueId);

					const char* what() const noexcept override;

					repo::lib::Container container;
					repo::lib::RepoUUID uniqueId;
				};
			}
		}
	}
}