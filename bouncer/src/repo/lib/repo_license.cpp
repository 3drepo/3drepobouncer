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

#include "repo_license.h"
#include "repo_log.h"
#include "datastructure/repo_uuid.h"
#include "repo_exception.h"
#include "repo_utils.h"

using namespace repo::lib;

#ifdef REPO_LICENSE_CHECK
#include "cryptolens/Error.hpp"

std::string LicenseValidator::instanceUuid;
std::string LicenseValidator::license;
std::unique_ptr<Cryptolens> LicenseValidator::cryptolensHandle;

std::string LicenseValidator::getInstanceUuid()
{
	instanceUuid = instanceUuid.empty() ? repo::lib::getEnvString(instanceUuidEnvVarName) : instanceUuid;
	if (instanceUuid.empty())
	{
		instanceUuid = repo::lib::RepoUUID::createUUID().toString();
	}
	return instanceUuid;
}

std::string LicenseValidator::getLicenseString()
{
	std::string licenseStr = repo::lib::getEnvString(licenseEnvVarName);
	if (licenseStr.empty())
	{
		std::string errMsg = "License not found, please ensure " + licenseEnvVarName + " is set.";
		throw repo::lib::RepoInvalidLicenseException(errMsg);
	}
	return licenseStr;
}

std::string LicenseValidator::getFormattedUtcTime(time_t timeStamp)
{
	struct tm  tstruct = *gmtime(&timeStamp);
	char buf[80];
	strftime(buf, sizeof(buf), "%Y/%m/%d %X", &tstruct);
	std::string formatedStr(buf);
	return formatedStr;
}

#endif

void LicenseValidator::activate()
{
#ifdef REPO_LICENSE_CHECK
	if (!instanceUuid.empty() || cryptolensHandle)
	{
		repoError << " Attempting to activate more than once, aborting activation";
		return;
	}

	license = getLicenseString();
	cryptolens::Error e;
	cryptolensHandle = std::unique_ptr<Cryptolens>(new Cryptolens(e));
	cryptolensHandle->signature_verifier.set_modulus_base64(e, pubKeyModulus);
	cryptolensHandle->signature_verifier.set_exponent_base64(e, pubKeyExponent);

	cryptolensHandle->machine_code_computer.set_machine_code(e, getInstanceUuid());

	cryptolens::optional<cryptolens::LicenseKey> licenseKey =
		cryptolensHandle->activate_floating
		(
			e,
			authToken,
			productId,
			license,
			floatingTimeIntervalSec
		);

	if (e)
	{
		cryptolens::ActivateError error = cryptolens::ActivateError::from_reason(e.get_reason());
		std::string errMsg = error.what();
		repoError << "License activation failed: " << errMsg;
		throw repo::lib::RepoInvalidLicenseException(errMsg);
	}
	else
	{
		bool licenseBlocked = licenseKey->get_block();
		bool licenseExpired = static_cast<bool>(licenseKey->check().has_expired(time(0)));
		if (!licenseExpired && !licenseBlocked)
		{
			repoTrace << activationSummaryBlock;
			repoTrace << "- session license ID: " << instanceUuid;
			repoTrace << "- server message: " << licenseKey->get_notes().has_value() ? licenseKey->get_notes().value() : "";
			repoTrace << "- server respose ok: true";
			repoTrace << "- license blocked: " << licenseBlocked;
			repoTrace << "- license expired: " << licenseExpired;
			repoTrace << "- license expiry on: " << getFormattedUtcTime(licenseKey->get_expires()) << " (UTC)";
			if (licenseKey->get_activated_machines().has_value()) repoTrace << "- #activated instances: " << licenseKey->get_activated_machines()->size();
			if (licenseKey->get_maxnoofmachines().has_value()) repoTrace << "- allowed instances: " << licenseKey->get_maxnoofmachines().value();
			repoInfo << "License activated";
			repoTrace << activationSummaryBlock;
		}
		else
		{
			std::string errMsg = licenseExpired ? "License expired." : " License is blocked";
			repoError << "License activation failed: " << errMsg;
			repoTrace << activationSummaryBlock;
			deactivate();
			throw repo::lib::RepoInvalidLicenseException(errMsg);
		}
	}
#endif
}

void LicenseValidator::deactivate()
{
#ifdef REPO_LICENSE_CHECK
	if (instanceUuid.empty() || !cryptolensHandle)
	{
		repoError << " Attempting to deactivate without activation, aborting deactivation";
		return;
	}

	cryptolens::Error e;
	cryptolensHandle->deactivate(
		e,
		authToken,
		productId,
		license,
		instanceUuid,
		true);

	if (e)
	{
		repoTrace << deactivationSummaryBlock;
		cryptolens::ActivateError error = cryptolens::ActivateError::from_reason(e.get_reason());
		repoTrace << "- server message: " << error.what();
		repoTrace << "- session license ID: " << instanceUuid;
		repoTrace << "- deactivation result: session not removed from license. Error trying to deactivate license. ";
		repoTrace << " this allocation will time out in less than " << floatingTimeIntervalSec << " seconds";
		repoTrace << deactivationSummaryBlock;
		repoError << "License deactivation failed: " << error.what();
	}
	else
	{
		repoInfo << "License allocation relinquished";
		repoTrace << deactivationSummaryBlock;
	}

	cryptolensHandle.reset();
	instanceUuid.clear();
#endif
}
