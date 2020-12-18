// eslint-disable-next-line security/detect-child-process
const { spawn } = require('child_process');

const { ERRCODE_TIMEOUT } = require('../constants/errorCodes');
const logger = require('./logger');
const { timeoutMS } = require('./config').config;

const run = (exe, params, codesAsSuccess = []) => new Promise((resolve, reject) => {
	logger.info(`Executing command: ${exe} ${params.join(' ')}`);
	const cmdExec = spawn(exe, params);
	let isTimeout = false;
	cmdExec.on('close', (code) => {
		if (isTimeout) {
			reject(ERRCODE_TIMEOUT);
		} else if (code === 0 || codesAsSuccess.includes(code)) {
			resolve(code);
		} else {
			reject(code);
		}
	});

	cmdExec.stdout.on('data', (data) => logger.verbose(`[STDOUT]: ${data}`));
	cmdExec.stderr.on('data', (data) => logger.verbose(`[STDERR]: ${data}`));

	setTimeout(() => {
		isTimeout = true;
		cmdExec.kill();
	}, timeoutMS);
});

module.exports = run;
