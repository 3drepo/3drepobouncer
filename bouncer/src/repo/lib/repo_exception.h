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

#include <string>
#include <exception>
#include "repo/error_codes.h"
#include "repo/repo_bouncer_global.h"

namespace repo {
	namespace lib {
		REPO_API_EXPORT class RepoException : public std::exception {
		public:
			RepoException(const std::string& msg);

			char const* what() const throw();

			/*
			 * Returns the code from error_codes.h, that best describes this exception.
			 * The default is REPOERR_UNKNOWN_ERR, if not set by a subclass.
			 */
			REPO_API_EXPORT int repoCode() const;

			/*
			* Returns a string describing this exception and all nested exceptions, if
			* any. Use this instead of 'what' in the outermost exception handlers.
			*/
			REPO_API_EXPORT std::string printFull() const;

		protected:
			int errorCode;
			const std::string errMsg;
		};

		REPO_API_EXPORT class RepoInvalidLicenseException : public RepoException {
		public:
			RepoInvalidLicenseException(const std::string& msg);
		};

		REPO_API_EXPORT class RepoGeometryProcessingException : public RepoException {
		public:
			RepoGeometryProcessingException(const std::string& msg);
		};

		// Default error code is REPOERR_LOAD_SCENE_FAIL, but can be overridden for
		// more detailed reporting.
		REPO_API_EXPORT class RepoSceneProcessingException : public RepoException {
		public:
			RepoSceneProcessingException(const std::string& msg);
			RepoSceneProcessingException(const std::string& msg, int errorCode);
		};

		REPO_API_EXPORT class RepoImporterUnavailable : public RepoException {
		public:
			RepoImporterUnavailable(const std::string& message, int code);
		};

		REPO_API_EXPORT class RepoBSONException : public RepoException {
		public:
			RepoBSONException(const std::string& msg);
		};

		REPO_API_EXPORT class RepoFieldNotFoundException : public RepoBSONException {
		public:
			RepoFieldNotFoundException(const std::string& fieldName);
		};

		REPO_API_EXPORT class RepoFieldTypeException : public RepoBSONException {
		public:
			RepoFieldTypeException(const std::string& fieldName);
		};
	}
}