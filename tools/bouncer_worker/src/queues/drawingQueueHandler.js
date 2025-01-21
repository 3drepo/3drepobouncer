/**
 * Copyright (C) 2024 3D Repo Ltd
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

const Path = require('path');
const {
	callbackQueueSpecified,
	logDirExists,
	sharedDirExists } = require('./common');
const { config } = require('../lib/config');
const { runBouncerCommand } = require('../tasks/bouncerClient');
const { generateSVG } = require('../tasks/imageProcessing');
const { ERRCODE_OK, ERRCODE_BOUNCER_CRASH, ERRCODE_REPO_LICENCE_INVALID } = require('../constants/errorCodes');
const { PROCESSING } = require('../constants/statuses');
const { messageDecoder } = require('../lib/messageDecoder');
const logger = require('../lib/logger');
const processMonitor = require('../lib/processMonitor');
const Utils = require('../lib/utils');

const Handler = {};
const logLabel = { label: 'DRAWINGQ' };

const generateTaskProfile = (user, model, database, rid, format, size) => ({
	...Utils.gatherProcessInformation(
		user,
		model,
		database,
		logLabel.label, // queue
		config.repoLicense,
		rid,
	),
	Revision: rid,
	FileFormat: format,
	FileSize: size,
});

Handler.onMessageReceived = async (cmd, rid, callback) => {
	const ridString = rid.toString();
	const logDir = Path.join(config.logging.taskLogDir, ridString);
	const { errorCode, database, model, user, cmdParams, format, size, file } = messageDecoder(cmd);

	if (errorCode) {
		callback(JSON.stringify({ value: errorCode }));
		return;
	}

	// A short pause before we update the model status as sometimes this happens so fast that
	// it preceeds the amqp confirming to io that the item has been queued.
	await Utils.sleep(100);
	callback(JSON.stringify({
		status: PROCESSING,
		database,
		project: model,
	}));

	const returnMessage = {
		value: ERRCODE_OK,
		database,
		project: model,
		user,
	};

	try {
		const procInfo = generateTaskProfile(user, model, database, ridString, format, size);

		if (format === '.pdf') {
			const svgPath = Path.join(logDir, `${ridString}.svg`);
			await generateSVG(file, svgPath, procInfo);
			cmdParams.push(svgPath);
		}

		returnMessage.value = await runBouncerCommand(logDir, cmdParams, procInfo);
		logger.verbose('Preparing to sendReport', ridString, logLabel);
		await processMonitor.sendReport(ridString);

		callback(JSON.stringify(returnMessage));
	} catch (err) {
		switch (err) {
			case ERRCODE_REPO_LICENCE_INVALID:
				logger.error('Failed to run 3drepobouncer: Invalid 3D Repo license', logLabel);
				await processMonitor.clearReport(ridString);
				await Utils.sleep(config.rabbitmq.maxWaitTimeMS);
				throw err;
			default:
				logger.error(`Import drawing error: ${err.message || err}`, logLabel);
				await processMonitor.sendReport(ridString);
				returnMessage.value = err || ERRCODE_BOUNCER_CRASH;
				callback(JSON.stringify(returnMessage));
		}
	}
};

Handler.validateConfiguration = (label) => callbackQueueSpecified(label)
	&& logDirExists(label)
	&& sharedDirExists(label);

Handler.prefetchCount = config.rabbitmq.drawing_prefetch;

module.exports = Handler;
