/**
 * Copyright (C) 2020 3D Repo Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

const fs = require('fs');
const readline = require('readline');
const { config, configPath } = require('../lib/config');
const { ERRCODE_BUNDLE_GEN_FAIL, ERRCODE_UNITY_LICENCE_INVALID, ERRCODE_REPO_LICENCE_INVALID } = require('../constants/errorCodes');
const run = require('../lib/runCommand');
const logger = require('../lib/logger');
const { getCurrentDateTimeAsString } = require('../lib/utils');

const UnityHandler = {};

const logLabel = { label: 'UNITY' };

const checkLicenceError = (log) => new Promise((resolve) => {
	// We want to check if the unity process failed with licensing error.
	// This seems to fail with an error code on windows, but returns success on linux
	// So the only way is to look through the log and grep for the error message - super ad hoc!
	const readStream = fs.createReadStream(log);
	const rl = readline.createInterface({
		input: readStream,
	});
	let licenseError = false;

	rl.on('line', (line) => {
		if (
			line.includes('Current license is invalid and cannot be activated')
				|| line.includes('Unity has not been activated with a valid License')
				|| line.includes('Failed to activate/update license')
		) {
			licenseError = true;
			rl.close();
		}
	});

	rl.on('close', () => {
		resolve(licenseError);
	});
});

UnityHandler.generateAssetBundles = async (database, model, rid, logDir, processInformation) => {
	const unityCommand = config.unity.batPath;
	const dateStr = getCurrentDateTimeAsString();
	const unityLog = `${logDir}/unity_${dateStr}.log`;
	const unityCmdParams = [
		config.unity.project,
		configPath,
		database,
		model,
		rid,
		unityLog,
	];

	if (config.repoLicense) {
		process.env.REPO_LICENSE = config.repoLicense;
	}

	try {
		const retVal = await run(unityCommand, unityCmdParams, { logLabel }, processInformation);
		if (await checkLicenceError(unityLog)) {
			throw ERRCODE_UNITY_LICENCE_INVALID;
		}

		return retVal;
	} catch (err) {
		logger.info(`Failed to execute unity command: ${err}`, logLabel);
		switch (err) {
			case ERRCODE_UNITY_LICENCE_INVALID:
			case ERRCODE_REPO_LICENCE_INVALID:
				throw err;
			default:
				throw await checkLicenceError(unityLog) ? ERRCODE_UNITY_LICENCE_INVALID : ERRCODE_BUNDLE_GEN_FAIL;
		}
	}
};

UnityHandler.validateUnityConfigurations = () => {
	if (!(config.unity && config.unity.project)) {
		logger.error('unity.project is not specified!', logLabel);
		return false;
	}

	if (!config.unity.project) {
		logger.error('unity.project is not specified!', logLabel);
		return false;
	}
	return true;
};

module.exports = UnityHandler;
