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

namespace Licensing
{
	// definitions for class static variables
	repo::lib::RepoUUID LicenseValidator::instanceId;
	std::string LicenseValidator::licenseStr;
	std::unique_ptr<Cryptolens> LicenseValidator::cryptolensHandle;

	void LicenseValidator::RunActivation()
	{
#ifdef REPO_LICENSE_CHECK
		// only run once 
		if (!instanceId.isDefaultValue() && cryptolensHandle) return;

		// Setting up the handle
		licenseStr = GetLicenseString();
		cryptolens::Error e;
		cryptolensHandle = std::make_unique<Cryptolens>(e);
		cryptolensHandle->signature_verifier.set_modulus_base64(e, pubKeyModulus);
		cryptolensHandle->signature_verifier.set_exponent_base64(e, pubKeyExponent);
		// Using machine code to store the instance UUID instead, as per
		// https://help.cryptolens.io/licensing-models/containers
		instanceId = repo::lib::RepoUUID::createUUID();
		cryptolensHandle->machine_code_computer.set_machine_code(e, instanceId.toString());

		// License activation 
		cryptolens::optional<cryptolens::LicenseKey> licenseKey =
			cryptolensHandle->activate_floating
			(
				e, // Object used for reporting if an error occured
				authToken, // Cryptolens Access Token
				productId, // Product key associated with the 3drepo.io pipeline
				licenseStr, // License Key
				floatingTimeIntervalSec // The amount of time the user has to wait before an actived machine (or session in our case) is taken off the license.
			);

		// dealing with early bail out scenarios
		repoInfo << "****License activation summary****";
		if (e)
		{
			cryptolens::ActivateError error = cryptolens::ActivateError::from_reason(e.get_reason());
			repoInfo << "- server message: " << error.what();
			repoInfo << "- server respose ok: false";
			repoInfo << "- session not added to license";
			throw repo::lib::RepoInvalidLicenseException();
		}
		else if (!licenseKey)
		{
			repoInfo << "- server respose ok: false";
			repoInfo << "- session not added to license. Error license LicenseKey is null";
			throw repo::lib::RepoInvalidLicenseException();
		}
		// dealing with the license check logic once all info is present
		else
		{
			// printing out the result
			std::string notes = licenseKey->get_notes().has_value() ?
				licenseKey->get_notes().value() : "";
			int noUsedInsances = licenseKey->get_activated_machines().has_value() ?
				licenseKey->get_activated_machines()->size() : -1;
			int maxInstances = licenseKey->get_maxnoofmachines().has_value() ?
				licenseKey->get_maxnoofmachines().value() : -1;
			bool licenseBlocked = licenseKey->get_block();
			bool licenseExpired = licenseKey->check().has_expired(time(0)) ? true : false;
			repoInfo << "- session license ID: " << instanceId.toString();
			repoInfo << "- server message: " << notes;
			repoInfo << "- server respose ok: true";
			repoInfo << "- license blocked: " << licenseBlocked;
			repoInfo << "- license expired: " << licenseExpired;
			repoInfo << "- license expiry on: " <<
				GetFormattedUtcTime(licenseKey->get_expires()) << " (UTC)";
			if (noUsedInsances >= 0) repoInfo << "- activated instances: " << noUsedInsances;
			if (maxInstances > 0) repoInfo << "- allowed instances: " << maxInstances;

			// handle result
			bool allCheck = !licenseExpired && !licenseBlocked;
			if (allCheck)
			{
				repoInfo << "- activation result: session succesfully added to license";
			}
			else
			{
				repoInfo << "- activation result: session activation failed";
				throw repo::lib::RepoInvalidLicenseException();
			}
		}
#endif
	}

	void LicenseValidator::RunDeactivation()
	{
#ifdef REPO_LICENSE_CHECK
		// only deactivate if we have succesfully activated
		if (instanceId.isDefaultValue() && cryptolensHandle) return;

		cryptolens::Error e;
		cryptolensHandle->deactivate(
			e,
			authToken,
			productId,
			licenseStr,
			true);

		// dealing with the error in deactivation
		repoInfo << "****License deactivation summary****";
		if (e)
		{
			cryptolens::ActivateError error = cryptolens::ActivateError::from_reason(e.get_reason());
			repoInfo << "- server message: " << error.what();
			repoInfo << "- deactivation result: session not removed from license. " <<
				"Error trying to deactivate license, " <<
				"this instance will be taken off the license in less than " <<
				floatingTimeIntervalSec << " seconds";
		}
		else
		{
			repoInfo << "- deactivation result: session succesfully removed from license";
		}
#endif
	}

	std::string LicenseValidator::GetLicenseString()
	{
		std::string licenseStr;
		char* licenseStrPtr = getenv(licenseEnvVarName.c_str());
		if (licenseStrPtr && strlen(licenseStrPtr) > 0)
		{
			licenseStr = std::string(licenseStrPtr);
		}
		else
		{
			std::stringstream ss;
			ss << "License not found, expected to find it in this " <<
				"environment variable: " <<
				licenseEnvVarName;
			throw repo::lib::RepoInvalidLicenseException(ss.str());
		}
		return licenseStr;
	}

	std::string LicenseValidator::GetFormattedUtcTime(time_t timeStamp)
	{
		struct tm  tstruct = *gmtime(&timeStamp);
		char buf[80];
		strftime(buf, sizeof(buf), "%Y/%m/%d %X", &tstruct);
		std::string formatedStr(buf);
		return formatedStr;
	}

}

