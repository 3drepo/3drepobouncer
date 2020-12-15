const logger = require("../lib/logger");
const { config, configPath} = require("../lib/config");
const path = require("path");
const run = require("../lib/runCommand");
const { BOUNCER_SOFT_FAILS } = require("../constants/errorCodes");

const bouncerClientPath = path.normalize(config.bouncer.path);


const setBouncerEnvars = (logDir) => {
	if (config.bouncer.envars) {
		Object.keys(config.bouncer.envars).forEach((key) => {
			process.env[key] = config.bouncer.envars[key];
		});
	}

	if(logDir) {
		process.env['REPO_LOG_DIR']= logDir ;
	}
}

const BouncerHandler = {};

BouncerHandler.testClient = async () => {
	logger.info("Checking status of client...");

	setBouncerEnvars();

	const cmdParams = [
		configPath,
		"test"
	];

	try {
		await run(bouncerClientPath, cmdParams);
		logger.info("Bouncer call passed");
	} catch (code) {
		logger.error(`Bouncer call errored (Error code: ${code})`);
	}
}

BouncerHandler.runCommand = async (logDir, cmdParams) => {
	setBouncerEnvars(logDir);
	return await run(bouncerClientPath, cmdParams, BOUNCER_SOFT_FAILS);
}

BouncerHandler.generateTreeStash = async (logDir, database, modelId, stashType, rev = "all") => {
	const cmd = `genStash ${database} ${modelId} ${stashType} ${rev}`;
	return BouncerHandler.runCommand(logDir, cmd);
}



module.exports = BouncerHandler;
