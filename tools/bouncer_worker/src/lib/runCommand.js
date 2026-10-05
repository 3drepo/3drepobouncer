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

const { ERRCODE_TIMEOUT, ERRCODE_UNKNOWN_ERROR } = require('../constants/errorCodes');
const kill = require('tree-kill');
const logger = require('./logger');
const processMonitor = require('./processMonitor');
// eslint-disable-next-line security/detect-child-process
const { spawn } = require('child_process');
const { timeoutMS } = require('./config').config;

const run = (
	exe,
	params,
	{ codesAsSuccess = [], verbose = true, logLabel },
	processInformation,
) => new Promise((resolve, reject) => {
	if (verbose) logger.info(`Executing command: "${exe}" ${params.join(' ')}`, logLabel);
	const cmdExec = spawn(`"${exe}"`, params, { shell: true });
	if (processInformation) processMonitor.startMonitor(processInformation);
	let isTimeout = false;
	let hasTerminated = false;
	cmdExec.on('close', (code, signal) => {
		hasTerminated = true;
		// eslint-disable-next-line max-len
		if (processInformation) processMonitor.stopMonitor(processInformation.Rid, (isTimeout ? ERRCODE_TIMEOUT : code));
		if (verbose) {
			logger.info(`Command executed. Code: ${isTimeout ? 'TIMEDOUT' : code} signal: ${signal}`, logLabel);
		}
		if (isTimeout) {
			reject(ERRCODE_TIMEOUT);
		} else if (code === 0 || codesAsSuccess.includes(code)) {
			resolve(code);
		} else {
			// NOTE: for some reason we're seeing code is null in linux. using -1 when that happens
			logger.info(`exiting with ERRCODE_UNKNOWN_ERROR: ${code} signal: ${signal}`, logLabel);
			reject(code || ERRCODE_UNKNOWN_ERROR);
		}
	});

	cmdExec.stdout.on('data', (data) => logger.verbose(`[STDOUT]: ${data}`, logLabel));
	cmdExec.stderr.on('data', (data) => logger.verbose(`[STDERR]: ${data}`, logLabel));

	setTimeout(() => {
		isTimeout = true;
		if (!hasTerminated) {
			logger.info('Max processing time reached, terminating the process', logLabel);
			kill(cmdExec.pid);
		}
	}, timeoutMS);
});

module.exports = run;
