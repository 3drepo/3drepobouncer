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
	memoryReporting = { enabled: false, maxMemory: 0, processTime: 0 },
) => new Promise((resolve, reject) => {
	if (verbose) logger.info(`Executing command: ${exe} ${params.join(' ')} memoryReporting: ${memoryReporting.enabled}`, logLabel);
	const cmdExec = spawn(exe, params, { shell: true });
	if (memoryReporting.enabled) {
		processMonitor.startMonitor(cmdExec.pid);
	}
	let isTimeout = false;
	cmdExec.on('close', (code, signal) => {
		if (memoryReporting.enabled) {
			processMonitor.stopMonitor(cmdExec.pid);
		}
		if (verbose) {
			logger.info(`Command executed. Code: ${isTimeout ? 'TIMEDOUT' : code} signal: ${signal}`, logLabel);
			if (memoryReporting.enabled) { logger.info(`[processMonitor] elapsed: ${processMonitor.processTime} maxMemory: ${processMonitor.maxMemory}`, logLabel); }
		}
		if (isTimeout) {
			reject(ERRCODE_TIMEOUT);
		} else if (code === 0 || codesAsSuccess.includes(code)) {
			if (memoryReporting.enabled) {
				// eslint-disable-next-line no-param-reassign
				memoryReporting.maxMemory = processMonitor.maxMemory;
				// eslint-disable-next-line no-param-reassign
				memoryReporting.processTime = processMonitor.processTime;
			}
			resolve(code);
		} else {
			// NOTE: for some reason we're seeing code is null in linux. using -1 when that happens
			logger.info(`[runCommand] exiting with ERRCODE_UNKNOWN_ERROR: ${code} signal: ${signal}`, logLabel);
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
