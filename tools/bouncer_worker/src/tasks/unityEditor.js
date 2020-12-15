/**
 *	Copyright (C) 2020 3D Repo Ltd
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU Affero General Public License as
 *	published by the Free Software Foundation, either version 3 of the
 *	License, or (at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU Affero General Public License for more details.
 *
 *	You should have received a copy of the GNU Affero General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>
 */


const { config, configPath } = require("../lib/config");
const { ERRCODE_BUNDLE_GEN_FAIL } = require("../constants/errorCodes");
const run = require("../lib/runCommand");
const logger = require("../lib/logger");

const UnityHandler = {}

UnityHandler.generateAssetBundles = async (database, model, logDir) => {
	if(config.unity && config.unity.project) {
		const unityCommand = config.unity.batPath;
		const unityCmdParams = [
			config.unity.project,
			configPath,
			database,
			model,
			logDir
		];

		try {
			logger.info(`Running unity command: ${unityCommand} ${unityCmdParams.join(" ")}`);
			const code = await run(unityCommand, unityCmdParams);
			logger.info(`[SUCCESS] Executed unity command: ${unityCommand} ${unityCmdParams.join(" ")}`, code);
			return code;
		} catch (err) {
			logger.info(`[FAILED] Executed unity command: ${unityCommand} ${unityCmdParams.join(" ")}`, err);
			throw ERRCODE_BUNDLE_GEN_FAIL;
		}
	} else {
		logger.info("Unity generation is not configured to run, skipping...");
		return 0;
	}
};

module.exports = UnityHandler;
