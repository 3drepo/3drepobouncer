// eslint-disable-next-line security/detect-child-process
const { spawn } = require('child_process');
const { ERRCODE_TIMEOUT, ERRCODE_UNKNOWN_ERROR } = require('../constants/errorCodes');
const logger = require('./logger');
const processMonitor = require('./processMonitor');
const { timeoutMS } = require('./config').config;

const run = (
	exe,
	params,
	{ codesAsSuccess = [], verbose = true, logLabel },
	processInformation,
) => new Promise((resolve, reject) => {
	if (verbose) logger.info(`Executing command: ${exe} ${params.join(' ')}`, logLabel);
	const cmdExec = spawn(exe, params, { shell: true });
	if (processInformation) processMonitor.startMonitor(cmdExec.pid, processInformation);
	let isTimeout = false;
	cmdExec.on('close', (code, signal) => {
		if (processInformation) processMonitor.stopMonitor(cmdExec.pid, code);

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
		cmdExec.kill();
	}, timeoutMS);
});

module.exports = run;
