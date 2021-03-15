const path = require('path');
const logger = require('../lib/logger');
const { config, configPath } = require('../lib/config');
const run = require('../lib/runCommand');
const { BOUNCER_SOFT_FAILS } = require('../constants/errorCodes');

const bouncerClientPath = path.normalize(config.bouncer.path);

const setBouncerEnvars = (logDir) => {
	if (config.bouncer.envars) {
		Object.keys(config.bouncer.envars).forEach((key) => {
			process.env[key] = config.bouncer.envars[key];
		});
	}

	if (logDir) {
		process.env.REPO_LOG_DIR = logDir;
	}
};

const BouncerHandler = {};

BouncerHandler.testClient = async () => {
	const logLabel = { label: 'INIT' };
	logger.info('Checking status of client...', logLabel);

	setBouncerEnvars();

	const cmdParams = [
		configPath,
		'test',
	];

	try {
		await run(bouncerClientPath, cmdParams, { logLabel });
		logger.info('Bouncer call passed', logLabel);
	} catch (code) {
		logger.error(`Bouncer call errored (Error code: ${code})`, logLabel);
		throw code;
	}
};

BouncerHandler.runBouncerCommand = async (
	logDir,
	cmdParams,
	memoryReporting = { enabled: false, maxMemory: 0, processTime: 0 },
) => {
	setBouncerEnvars(logDir);
	return run(bouncerClientPath, cmdParams, { codesAsSuccess: BOUNCER_SOFT_FAILS, logLabel: { label: 'BOUNCER' } }, memoryReporting);
};

BouncerHandler.generateTreeStash = async (logDir, database, modelId, stashType, rev = 'all') => {
	const cmd = [configPath, 'genStash', database, modelId, stashType, rev];
	return BouncerHandler.runBouncerCommand(logDir, cmd);
};

module.exports = BouncerHandler;
