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
const { ERRCODE_ARG_FILE_FAIL, ERRCODE_BUNDLE_GEN_FAIL } = require('../constants/errorCodes');
const { UNITY_PROCESSING } = require('../constants/statuses');
const logger = require('../lib/logger');
const Elastic = require('../lib/elastic');

const logLabel = { label: 'UNITYQ' };

// eslint-disable-next-line max-len
const processUnity = async (database, model, user, logDir, modelImportErrCode) => {
	const returnMessage = {
		value: modelImportErrCode,
		database,
		project: model,
		user,
	};

	try {
		if (database && model) {
			const memoryReporting = { enabled: config.elastic.memoryReporting, maxMemory: 0, processTime: 0 };
			await generateAssetBundles(database, model, logDir, memoryReporting);
			if (memoryReporting.enabled) {
				const elasticBody = {
					Teamspace: user,
					Model: model,
					Database: database,
					MaxMemory: memoryReporting.maxMemory,
					ProcessTime: memoryReporting.processTime,
					DateTime: Date.now(),
					FileType: 'Unity',
					FileSize: null,
					Process: 'Unity',
					ReturnCode: returnMessage.value,
				};
				try { await Elastic.createBouncerRecord(elasticBody); } catch (err) {
					logger.error(`[processMonitor] Failed to create Elastic record: ${err.message || err}`);
				}
				logger.verbose(`[processMonitor] ProcessTime: ${elasticBody.ProcessTime} MaxMemory: ${elasticBody.MaxMemory} Process: ${elasticBody.Process} `, logLabel);
			}
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
	const { database, project, user, value } = JSON.parse(cmd);
	const logDir = `${config.logging.taskLogDir}/${rid.toString()}/`;

	callback(JSON.stringify({
		status: UNITY_PROCESSING,
		database,
		project,
	}));
	const message = await processUnity(database, project, user, logDir, value);
	callback(JSON.stringify(message));
};

Handler.validateConfiguration = (label) => callbackQueueSpecified(label)
		&& unityQueueSpecified(label)
		&& logDirExists(label)
		&& validateUnityConfigurations();

module.exports = Handler;
