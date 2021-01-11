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
const { runBouncerCommand } = require('../tasks/bouncerClient');
const { ERRCODE_OK, ERRCODE_BOUNCER_CRASH } = require('../constants/errorCodes');
const { messageDecoder } = require('../lib/messageDecoder');
const logger = require('../lib/logger');

const Handler = {};

Handler.onMessageReceived = async (cmd, rid, callback) => {
	const logDir = `${config.logging.taskLogDir}/${rid.toString()}/`;
	const { errorCode, database, model, cmdParams } = messageDecoder(cmd);

	if (errorCode) {
		callback({ value: errorCode });
	} else {
		callback(JSON.stringify({
			status: 'processing',
			database,
			project: model,
		}));
	}

	const returnMessage = {
		value: ERRCODE_OK,
		database,
		project: model,
	};

	try {
		returnMessage.value = await runBouncerCommand(logDir, cmdParams);
		callback(JSON.stringify(returnMessage), config.rabbitmq.unity_queue);
		callback(JSON.stringify({
			status: 'queuing for unity service',
			database,
			project: model,
		}));
	} catch (err) {
		logger.error(`Import model error: ${err.message || err}`);
		returnMessage.value = err || ERRCODE_BOUNCER_CRASH;
		callback(JSON.stringify(returnMessage));
	}
};

Handler.validateConfiguration = () => {
	if (!config.rabbitmq.model_queue) {
		logger.error('rabbitmq.model_queue is not specified!');
		return false;
	}

	if (!config.rabbitmq.callback_queue) {
		logger.error('rabbitmq.callback_queue is not specified!');
		return false;
	}

	if (!config.rabbitmq.unity_queue) {
		logger.error('rabbitmq.unity_queue is not specified!');
		return false;
	}

	if (!config.logging.taskLogDir) {
		logger.error('logging.taskLogDir is not specified!');
		return false;
	}

	return true;
};

module.exports = Handler;
