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

	if (config.repoLicense) {
		process.env.REPO_LICENSE = config.repoLicense;
		process.env.REPO_INSTANCE_ID = config.instanceId;
	}
};

const BouncerHandler = {};

BouncerHandler.testClient = async () => {
	const logLabel = { label: 'INIT' };
	logger.info('Checking status of client...', logLabel);
	if (config.repoLicense) {
		logger.info(`Machine Instance ID is set to ${config.instanceId}`);
	}

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
	processInformation,
) => {
	setBouncerEnvars(logDir);
	return run(bouncerClientPath, cmdParams, { codesAsSuccess: BOUNCER_SOFT_FAILS, logLabel: { label: 'BOUNCER' } }, processInformation);
};

BouncerHandler.generateTreeStash = async (logDir, database, modelId, stashType, rev = 'all') => {
	const cmd = [configPath, 'genStash', database, modelId, stashType, rev];
	return BouncerHandler.runBouncerCommand(logDir, cmd);
};

module.exports = BouncerHandler;
