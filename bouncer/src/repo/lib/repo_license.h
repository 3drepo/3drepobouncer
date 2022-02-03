/**
*  Copyright (C) 2021 3D Repo Ltd
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

#include <memory>

#ifdef REPO_LICENSE_CHECK

#include "cryptolens/core.hpp"
#include "cryptolens/MachineCodeComputer_static.hpp"

namespace cryptolens = ::cryptolens_io::v20190401;
#if defined(_WIN32) || defined(_WIN64)
#include "cryptolens/Configuration_Windows.hpp"
using Cryptolens = cryptolens::basic_Cryptolens<cryptolens::Configuration_Windows_IgnoreExpires<cryptolens::MachineCodeComputer_static>>;
#else
#include "cryptolens/Configuration_Unix.hpp"
using Cryptolens = cryptolens::basic_Cryptolens<cryptolens::Configuration_Unix_IgnoreExpires<cryptolens::MachineCodeComputer_static>>;
#endif

#endif
namespace repo {
	namespace lib {
		class LicenseValidator
		{
#ifdef REPO_LICENSE_CHECK

		private:
			std::string license;
			std::string instanceUuid;
			std::unique_ptr<Cryptolens> cryptolensHandle;

			std::string getInstanceUuid();
			std::string getLicenseString();
			std::string getFormattedUtcTime(time_t timeStamp);

#endif

		public:
			void activate();
			void deactivate();
		};
	}
}
