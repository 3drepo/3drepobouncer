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
const { generateAssetBundles } = require('../tasks/unityEditor');
const { ERRCODE_ARG_FILE_FAIL, ERRCODE_BOUNCER_CRASH } = require('../constants/errorCodes');
const logger = require('../lib/logger');

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
		logger.error(`Failed to generate asset bundle: ${err.message || err}`);
		returnMessage.value = err || ERRCODE_BOUNCER_CRASH;
	}

	return returnMessage;
};

const onMessageReceived = async (cmd, rid, callback) => {
	const { database, project, value } = JSON.parse(cmd);
	const logDir = `${config.bouncer.log_dir}/${rid.toString()}/`;

	callback(JSON.stringify({
		status: 'creating asset bundles',
		database,
		project,
	}));

	const message = await processUnity(database, project, logDir, value);
	callback(JSON.stringify(message));
};

module.exports = { onMessageReceived };
