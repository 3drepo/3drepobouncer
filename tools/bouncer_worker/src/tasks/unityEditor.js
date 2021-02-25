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

const { config, configPath } = require('../lib/config');
const { ERRCODE_BUNDLE_GEN_FAIL } = require('../constants/errorCodes');
const run = require('../lib/runCommand');
const logger = require('../lib/logger');
const { getCurrentDateTimeAsString } = require('../lib/utils');

const UnityHandler = {};

const logLabel = { label: 'UNITY' };

UnityHandler.generateAssetBundles = async (database, model, logDir) => {
	const unityCommand = config.unity.batPath;
	const dateStr = getCurrentDateTimeAsString();
	const unityCmdParams = [
		config.unity.project,
		configPath,
		database,
		model,
		`${logDir}/unity_${dateStr}.log`,
		`${logDir}/run_unity_${dateStr}.log`,
	];

	try {
		return await run(unityCommand, unityCmdParams, { logLabel });
	} catch (err) {
		logger.info(`Failed to execute unity command: ${err}`, logLabel);
		throw ERRCODE_BUNDLE_GEN_FAIL;
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
