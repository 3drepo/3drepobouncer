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

const {
	callbackQueueSpecified,
	jobQueueSpecified,
	logDirExists,
	sharedDirExists } = require('./common');
const { importToyModel, validateToyImporterSettings } = require('../tasks/importToy');
const { ERRCODE_OK, ERRCODE_TOY_IMPORT_FAILED } = require('../constants/errorCodes');
const { config } = require('../lib/config');
const { generateTreeStash, runBouncerCommand } = require('../tasks/bouncerClient');
const { messageDecoder } = require('../lib/messageDecoder');
const logger = require('../lib/logger');

const Handler = {};

const logLabel = { label: 'JOBQ' };

const importToy = async ({ database, model, toyModelID, skipPostProcessing }, logDir) => {
	const returnMessage = {
		value: ERRCODE_OK,
		database,
		project: model,
	};

	try {
		await importToyModel(toyModelID, database, model, skipPostProcessing);

		if (!skipPostProcessing.tree) {
			logger.info('Toy model imported. Generating tree...', logLabel);
			await generateTreeStash(logDir, database, model, 'tree');
		}
	} catch (err) {
		logger.error(`importToy module error: ${err.message || err}`, logLabel);
		returnMessage.value = ERRCODE_TOY_IMPORT_FAILED;
		returnMessage.message = err.message || err;
	}

	return returnMessage;
};

const createFed = async ({ database, model, toyFed, cmdParams }, logDir) => {
	const returnMessage = {
		value: ERRCODE_OK,
		database,
		project: model,
	};
	try {
		returnMessage.value = await runBouncerCommand(logDir, cmdParams);
		if (toyFed) {
			await importToyModel(toyFed, database, model, { tree: 1 });
		}
	} catch (err) {
		logger.error(`Error generating federation: ${err.message || err}`, logLabel);
		returnMessage.value = toyFed ? ERRCODE_TOY_IMPORT_FAILED : err;
		returnMessage.message = err.message || err;
	}

	return returnMessage;
};

Handler.onMessageReceived = async (cmd, rid, callback) => {
	const logDir = `${config.logging.taskLogDir}/${rid.toString()}/`;
	const cmdMsg = messageDecoder(cmd);

	if (cmdMsg.errorCode) {
		callback(JSON.stringify({ value: cmdMsg.errorCode }));
		return;
	}

	if (cmdMsg.command === 'importToy') {
		const message = await importToy(cmdMsg, logDir);
		callback(JSON.stringify(message));
	} else {
		const message = await createFed(cmdMsg, logDir);
		callback(JSON.stringify(message));
	}
};

Handler.validateConfiguration = (label) => logDirExists(label)
		&& jobQueueSpecified(label)
		&& sharedDirExists(label)
		&& callbackQueueSpecified(label)
		&& validateToyImporterSettings();

module.exports = Handler;
