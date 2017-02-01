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
*  Project setting BSON
*/

#pragma once
#include "repo_bson.h"

namespace repo {
	namespace core {
		namespace model {
			// TODO: make into header only

			#define REPO_LABEL_IS_FEDERATION "federate"
			class REPO_API_EXPORT RepoProjectSettings : public RepoBSON
			{
			public:

				RepoProjectSettings() : RepoBSON() {}

				RepoProjectSettings(RepoBSON bson) : RepoBSON(bson){}

				~RepoProjectSettings() {}

				/**
				* Clone and merge new project settings into the existing info
				* @proj new info that needs ot be added/updated
				* @return returns a new project settings with fields updated
				*/
				RepoProjectSettings cloneAndMergeProjectSettings(const RepoProjectSettings &proj) const;

				/**
				 * Get the avatar height if present or default value if not.
				 * @brief getAvatarHeight
				 * @return returns avatar height as double.
				 */
				double getAvatarHeight() const
				{
					return getEmbeddedDouble(
						REPO_LABEL_PROPERTIES,
						REPO_LABEL_AVATAR_HEIGHT,
						(double)REPO_DEFAULT_PROJECT_AVATAR_HEIGHT);
				}

				/**
				* Get the description of the project for this settings
				* @return returns project description as string
				*/
				std::string getDescription() const
				{
					return getStringField(REPO_LABEL_DESCRIPTION);
				}

				/**
				* Get the owner of the project for this settings
				* @return returns owner name as string
				*/
				std::string getOwner() const
				{
					return getStringField(REPO_LABEL_OWNER);
				}

				double getPinSize() const
				{
					return getEmbeddedDouble(
						REPO_LABEL_PROPERTIES,
						REPO_LABEL_PIN_SIZE,
						REPO_DEFAULT_PROJECT_PIN_SIZE);
				}

				/**
				* Get the name of the project for this settings
				* @return returns project name as string
				*/
				std::string getProjectName() const
				{
					return getStringField(REPO_LABEL_ID);
				}

				double getSpeed() const
				{
					return getEmbeddedDouble(
						REPO_LABEL_PROPERTIES,
						REPO_LABEL_SPEED,
						REPO_DEFAULT_PROJECT_SPEED);
				}

				/**
				* Get the type of the project for this settings
				* @return returns project type as string
				*/
				std::string getType() const
				{
					return getStringField(REPO_LABEL_TYPE);
				}

				double getVisibilityLimit() const
				{
					return getEmbeddedDouble(
						REPO_LABEL_PROPERTIES,
						REPO_LABEL_VISIBILITY_LIMIT,
						REPO_DEFAULT_PROJECT_VISIBILITY_LIMIT);
				}

				double getZFar() const
				{
					return getEmbeddedDouble(
						REPO_LABEL_PROPERTIES,
						REPO_LABEL_ZFAR,
						REPO_DEFAULT_PROJECT_ZFAR);
				}

				/**
				* Get the zNear of the project for this settings
				* @return returns project zNear as double
				*/
				double getZNear() const
				{
					return getEmbeddedDouble(
						REPO_LABEL_PROPERTIES,
						REPO_LABEL_ZNEAR,
						REPO_DEFAULT_PROJECT_ZNEAR);
				}
			};
		}// end namespace model
	} // end namespace core
} // end namespace repo
