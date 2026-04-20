/**
 * Copyright (C) 2026 3D Repo Ltd
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

const path = require('path');
const { ERRCODE_OK } = require('../constants/errorCodes');
const { config, replaceSharedDirTag } = require('../lib/config');
const { runBouncerCommand } = require('../tasks/bouncerClient');
const { messageDecoder } = require('../lib/messageDecoder');
const logger = require('../lib/logger');
const { MSGTYPE_CLASH } = require('../constants/messageTypes');

const Handler = {};

const logLabel = { label: 'CLASHQ' };

Handler.onMessageReceived = async (cmd, rid, callback) => {
	const logDir = `${config.logging.taskLogDir}/${rid.toString()}/`;
	const { errorCode, teamspace, project, configFile, cmdParams } = messageDecoder(cmd);

	if (errorCode) {
		callback(JSON.stringify({ value: errorCode }));
		return;
	}

	// Use the CLI to specify where the results file should go. Each clash run
	// should have its own folder, so we don't need to generate a unique name.

	// The shared directory may be mapped to different locations on different
	// machines, so we should preserve the tag when reporting back the location
	// of the file. It is safe to use dirname with the tag. The tag must be
	// replaced on all paths for bouncer however.

	const resultsFile = path.join(path.dirname(configFile), 'results.json');

	const returnMessage = {
		value: ERRCODE_OK,
		results: resultsFile,
		teamspace,
		project,
		type: MSGTYPE_CLASH,
	};

	try {
		returnMessage.value = await runBouncerCommand(logDir, [...cmdParams, 'clash', replaceSharedDirTag(configFile), replaceSharedDirTag(resultsFile)]);
	} catch (err) {
		logger.error(`Error running clash detection: ${err.message || err}`, logLabel);
		returnMessage.value = err;
		returnMessage.message = err.message || err;
	}

	callback(JSON.stringify(returnMessage));
};

Handler.prefetchCount = config.rabbitmq.task_prefetch;

module.exports = Handler;
