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

const { callbackQueueSpecified, unityQueueSpecified, logDirExists } = require('./common');
const { config } = require('../lib/config');
const { generateAssetBundles, validateUnityConfigurations } = require('../tasks/unityEditor');
const { ERRCODE_ARG_FILE_FAIL, ERRCODE_UNITY_LICENCE_INVALID, ERRCODE_REPO_LICENCE_INVALID } = require('../constants/errorCodes');
const { UNITY_PROCESSING } = require('../constants/statuses');
const logger = require('../lib/logger');
const processMonitor = require('../lib/processMonitor');
const Utils = require('../lib/utils');

const logLabel = { label: 'UNITYQ' };

const processUnity = async (database, model, user, rid, logDir, modelImportErrCode) => {
	const returnMessage = {
		value: modelImportErrCode,
		database,
		project: model,
		user,
	};
	const ridString = rid.toString();
	try {
		if (database && model) {
			const processInformation = Utils.gatherProcessInformation(
				user,
				model,
				database,
				logLabel.label, // queue
				'unity',
				0,
				config.repoLicense,
				ridString,
			);
			await generateAssetBundles(database, model, rid, logDir, processInformation);
			await processMonitor.sendReport(ridString);
		} else {
			returnMessage.value = ERRCODE_ARG_FILE_FAIL;
		}
	} catch (err) {
		switch (err) {
			case ERRCODE_UNITY_LICENCE_INVALID:
				logger.error('Failed to generate asset bundle: Invalid unity license', logLabel);
				await processMonitor.clearReport(ridString);
				await Utils.sleep(config.rabbitmq.maxWaitTimeMS);
				throw err;
			case ERRCODE_REPO_LICENCE_INVALID:
				logger.error('Failed to generate asset bundle: Invalid 3D Repo license', logLabel);
				await processMonitor.clearReport(ridString);
				await Utils.sleep(config.rabbitmq.maxWaitTimeMS);
				throw err;
			default:
				await processMonitor.sendReport(ridString);
				logger.error(`Failed to generate asset bundle: ${err}`, logLabel);
				returnMessage.value = err;
				break;
		}
	}
	return returnMessage;
};

const Handler = {};

Handler.onMessageReceived = async (cmd, rid, callback) => {
	const { database, project, user, value } = JSON.parse(cmd);
	const logDir = `${config.logging.taskLogDir}/${rid.toString()}/`;

	callback(JSON.stringify({
		status: UNITY_PROCESSING,
		database,
		project,
	}));
	const message = await processUnity(database, project, user, rid, logDir, value);
	callback(JSON.stringify(message));
};

Handler.validateConfiguration = (label) => callbackQueueSpecified(label)
		&& unityQueueSpecified(label)
		&& logDirExists(label)
		&& validateUnityConfigurations();

module.exports = Handler;
