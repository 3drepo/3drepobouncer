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

#include "repo_log.h"
#include "datastructure/repo_uuid.h"
#include "repo_exception.h"
#include <memory>

#ifdef REPO_LICENSE_CHECK

#include "cryptolens/core.hpp"
#include "cryptolens/Error.hpp"
#include "cryptolens/Configuration_Windows.hpp"
#include "cryptolens/MachineCodeComputer_static.hpp"

namespace cryptolens = ::cryptolens_io::v20190401;

using Cryptolens = cryptolens::basic_Cryptolens<cryptolens::Configuration_Windows_IgnoreExpires<cryptolens::MachineCodeComputer_static>>;

#endif

namespace Licensing
{
	static const std::string activationSummaryBlock = "****License activation summary****";
	static const std::string deactivationSummaryBlock = "****License deactivation summary****";
	static const std::string licenseEnvVarName = "REPO_LICENSE";
	static const std::string instanceUuidEnvVarName = "REPO_INSTANCE_ID";
	static const std::string pubKeyModulus = LICENSE_RSA_PUB_KEY_MOD;
	static const std::string pubKeyExponent = LICENSE_RSA_PUB_KEY_EXP;
	static const std::string authToken = LICENSE_AUTH_TOKEN;
	static const int floatingTimeIntervalSec = LICENSE_TIMEOUT_SECONDS;
	static const int productId = LICENSE_PRODUCT_ID;

	class LicenseValidator
	{
	private:
		static std::string license;
		static std::string instanceUuid;

#ifdef REPO_LICENSE_CHECK

		static std::unique_ptr<Cryptolens> cryptolensHandle;

#endif

	public:
		static void RunActivation();
		static void RunDeactivation();

	private:
		static std::string GetInstanceUuid();
		static std::string GetLicenseString();
		static std::string GetFormattedUtcTime(time_t timeStamp);
	};
}