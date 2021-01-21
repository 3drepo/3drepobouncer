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

const { config } = require('../lib/config');
const { generateAssetBundles, validateUnityConfigurations } = require('../tasks/unityEditor');
const { ERRCODE_ARG_FILE_FAIL, ERRCODE_BUNDLE_GEN_FAIL } = require('../constants/errorCodes');
const { UNITY_PROCESSING } = require('../constants/statuses');
const logger = require('../lib/logger');

const logLabel = { label: 'UNITYQ' };

const processUnity = async (database, model, logDir, modelImportErrCode) => {
	const returnMessage = {
		value: modelImportErrCode,
		database,
		project: model,
	};

	try {
		if (database && model) {
			await generateAssetBundles(database, model, logDir);
		} else {
			returnMessage.value = ERRCODE_ARG_FILE_FAIL;
		}
	} catch (err) {
		logger.error(`Failed to generate asset bundle: ${err.message || err}`, logLabel);
		returnMessage.value = ERRCODE_BUNDLE_GEN_FAIL;
	}

	return returnMessage;
};

const Handler = {};

Handler.onMessageReceived = async (cmd, rid, callback) => {
	const { database, project, value } = JSON.parse(cmd);
	const logDir = `${config.logging.taskLogDir}/${rid.toString()}/`;

	callback(JSON.stringify({
		status: UNITY_PROCESSING,
		database,
		project,
	}));

	const message = await processUnity(database, project, logDir, value);
	callback(JSON.stringify(message));
};

Handler.validateConfiguration = () => {
	if (!(config.rabbitmq && config.rabbitmq.callback_queue)) {
		logger.error('rabbitmq.callback_queue is not specified!', logLabel);
		return false;
	}

	if (!config.rabbitmq.unity_queue) {
		logger.error('rabbitmq.unity_queue is not specified!', logLabel);
		return false;
	}

	if (!(config.logging && config.logging.taskLogDir)) {
		logger.error('config.logging.taskLogDir is not specified!', logLabel);
		return false;
	}

	return validateUnityConfigurations();
};

module.exports = Handler;
