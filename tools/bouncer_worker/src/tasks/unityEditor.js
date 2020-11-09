const { config, configFullPath } = require("../lib/config");
const { ERRCODE_BUNDLE_GEN_FAIL } = require("../constants/errorCodes");
const run = require("../lib/runCommand");

const UnityHandler = {}

UnityHandler.generateAssetBundles = async (database, project, logDir) => {
	if(config.unity && config.unity.project) {
		const unityCommand = config.unity.batPath;
		const unityCmdParams = [
			config.unity.project,
			configFullPath,
			commandArgs.database,
			commandArgs.project,
			logDir
		];

		try {
			logger.info(`Running unity command: ${unityCommand} ${unityCmdParams.join(" ")}`);
			const code = await run(unityCommand, unityCmdParams);
			logger.info(`[SUCCESS] Executed unity command: ${unityCommand} ${unityCmdParams.join(" ")}`, unityCode);
			return code;
		} catch (err) {
			logger.info(`[FAILED] Executed unity command: ${unityCommand} ${unityCmdParams.join(" ")}`, unityCode);
			throw ERRCODE_BUNDLE_GEN_FAIL;
		}
	} else {
		logger.info("Unity generation is not configured to run, skipping...");
		return 0;
	}
};

module.exports = UnityHandler;
