const logger = require("../lib/logger");
const { config, configPath} = require("../lib/config");
const path = require("path");
const run = require("../lib/runCommand");

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

const testClient = async () => {
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


module.exports = {
	testClient
}
