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
#include "repo_utils.h"

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

#ifdef REPO_LICENSE_CHECK

	static const std::string activationSummaryBlock = "****License activation summary****";
	static const std::string deactivationSummaryBlock = "****License deactivation summary****";
	static const char * licenseEnvVarName = "REPO_LICENSE";
	static const char * instanceUuidEnvVarName = "REPO_INSTANCE_ID";

	static const std::string pubKeyModulus = LICENSE_RSA_PUB_KEY_MOD;
	static const std::string pubKeyExponent = LICENSE_RSA_PUB_KEY_EXP;
	static const std::string authToken = LICENSE_AUTH_TOKEN;
	static const int floatingTimeIntervalSec = LICENSE_TIMEOUT_SECONDS;
	static const int productId = LICENSE_PRODUCT_ID;

#endif


	/**
	* Class that performs license checks given an license key taken from an
	* environment variable.
	* 
	* The class implements a floating license model:
	* https://help.cryptolens.io/licensing-models/floating
	* Supports only one active session per process
	* 
	* Intended flow:
	* 
	* 1. RunActivation -> attempts activation, retains session data if succesful,
	* otherwise throws RepoInvalidLicenseException
	* 
	* 2. Perfom license protected processing
	* 
	* 3. RunDeactivation -> attempts to deactivate the activated session id. The
	* session data will always be cleared
	* 
	* In all other scenarios the function will do nothing
	*/
	class LicenseValidator
	{

#ifdef REPO_LICENSE_CHECK

	private:
		static std::string license;
		static std::string instanceUuid;
		static std::unique_ptr<Cryptolens> cryptolensHandle;

		static std::string GetInstanceUuid();
		static std::string GetLicenseString();
		static std::string GetFormattedUtcTime(time_t timeStamp);
		static void Reset();

#endif

	public:
		static void RunActivation();
		static void RunDeactivation();
	};
}