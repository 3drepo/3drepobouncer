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
	logDirExists,
	sharedDirExists } = require('./common');
const { ERRCODE_OK } = require('../constants/errorCodes');
const { config } = require('../lib/config');
const { runBouncerCommand } = require('../tasks/bouncerClient');
const { messageDecoder } = require('../lib/messageDecoder');
const logger = require('../lib/logger');

const Handler = {};

const logLabel = { label: 'JOBQ' };

const createFed = async ({ database, model, cmdParams }, logDir) => {
	const returnMessage = {
		value: ERRCODE_OK,
		database,
		project: model,
	};
	try {
		returnMessage.value = await runBouncerCommand(logDir, cmdParams);
	} catch (err) {
		logger.error(`Error generating federation: ${err.message || err}`, logLabel);
		returnMessage.value = err;
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

	const message = await createFed(cmdMsg, logDir);
	callback(JSON.stringify(message));
};

Handler.validateConfiguration = (label) =>
		callbackQueueSpecified(label)
		&& logDirExists(label)
		&& sharedDirExists(label);

module.exports = Handler;
