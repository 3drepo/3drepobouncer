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

static const std::string licenseEnvVarName = "REPO_LICENSE";
static const std::string instanceUuidEnvVarName = "REPO_INSTANCE_ID";

static const std::string pubKeyModulus = LICENSE_RSA_PUB_KEY_MOD;
static const std::string pubKeyExponent = LICENSE_RSA_PUB_KEY_EXP;
static const std::string authToken = LICENSE_AUTH_TOKEN;
static const int floatingTimeIntervalSec = LICENSE_TIMEOUT_SECONDS;
static const int productId = LICENSE_PRODUCT_ID;

std::string LicenseValidator::getInstanceUuid()
{
	instanceUuid = instanceUuid.empty() ? repo::lib::getEnvString(instanceUuidEnvVarName) : instanceUuid;
	if (instanceUuid.empty())
	{
		instanceUuid = repo::lib::RepoUUID::createUUID().toString();
		repoInfo << instanceUuidEnvVarName << " is not set. Setting machine instance ID to " << instanceUuid;
	}
	return instanceUuid;
}

std::string LicenseValidator::getLicenseString()
{
	std::string licenseStr = repo::lib::getEnvString(licenseEnvVarName);
	if (licenseStr.empty())
	{
		std::string errMsg = "License not found, please ensure " + licenseEnvVarName + " is set.";
		repoError << errMsg;
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

bool LicenseValidator::sendActivateRequest(bool verbose) {
	cryptolens::Error e;
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
		return false;
	}
	else
	{
		bool licenseBlocked = licenseKey->get_block();
		bool licenseExpired = static_cast<bool>(licenseKey->check().has_expired(time(0)));
		if (!licenseExpired && !licenseBlocked)
		{
			if (verbose) {
				repoTrace << "****License activation summary****";
				repoTrace << "- session license ID: " << instanceUuid;
				repoTrace << "- server message: " << (licenseKey->get_notes().has_value() ? licenseKey->get_notes().value() : "");
				repoTrace << "- server respose ok: true";
				repoTrace << "- license blocked: " << licenseBlocked;
				repoTrace << "- license expired: " << licenseExpired;
				repoTrace << "- license expiry on: " << getFormattedUtcTime(licenseKey->get_expires()) << " (UTC)";
				if (licenseKey->get_activated_machines().has_value()) repoTrace << "- #activated instances: " << licenseKey->get_activated_machines()->size();
				if (licenseKey->get_maxnoofmachines().has_value()) repoTrace << "- allowed instances: " << licenseKey->get_maxnoofmachines().value();
				repoInfo << "License activated";
				repoTrace << "**********************************";
			}
			return true;
		}
		else
		{
			std::string errMsg = licenseExpired ? "License expired." : " License is blocked";
			repoError << "License activation failed: " << errMsg;
			deactivate();
			return false;
		}
	}
}

void LicenseValidator::runHeartBeatLoop() {
	auto interval = floatingTimeIntervalSec <= 10 ? floatingTimeIntervalSec / 2 : 5;
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(interval));
		if (!sendHeartBeat.load()) {
			repoTrace << "Signal received to stop heart beat";
			break;
		}
		auto start = std::chrono::high_resolution_clock::now();
		if (!sendActivateRequest()) {
			repoError << "Heart beat to license server was not acknowledged";
			break;
		}
		auto stop = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
		repoTrace << "Heart beat acknowledged ["<< duration<< "ms]";
	}
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

	if (e || !sendActivateRequest(true)) {
		std::string errMsg = "Failed to activate license";
		if (e) {
			cryptolens::ActivateError error = cryptolens::ActivateError::from_reason(e.get_reason());
			errMsg = error.what();
		}

		throw repo::lib::RepoInvalidLicenseException(errMsg);
	}

	// launch a thread to keep sending heart beats
	auto thread = new std::thread([](LicenseValidator *obj) {obj->runHeartBeatLoop();}, this);
	heartBeatThread = std::unique_ptr<std::thread>(thread);

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

	if (heartBeatThread) {
		repoInfo << "Waiting for heart beat thread to finish...";
		sendHeartBeat.store(false);
		heartBeatThread->join();
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
		repoTrace << "****License deactivation summary****";
		cryptolens::ActivateError error = cryptolens::ActivateError::from_reason(e.get_reason());
		repoTrace << "- server message: " << error.what();
		repoTrace << "- session license ID: " << instanceUuid;
		repoTrace << "- deactivation result: session not removed from license. Error trying to deactivate license. ";
		repoTrace << " this allocation will time out in less than " << floatingTimeIntervalSec << " seconds";
		repoTrace << "************************************";
		repoTrace << "License deactivation failed: " << error.what();
	}

	cryptolensHandle.reset();
	instanceUuid.clear();
#endif
}