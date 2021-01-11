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
const fs = require('fs');
const yargs = require('yargs/yargs');
const { hideBin } = require('yargs/helpers');
const { exitApplication } = require('./utils');

const Config = {};

const parseParameters = () => {
	const args = yargs(hideBin(process.argv));
	return args.option('config', {
		describe: 'specify the path to a custom configuration file (default: config.json at root level)',
	}).option('runOnceOnQueue', {
		describe: 'Exit upon processing a single task on the specified queue [job|model|unity]',
		choice: ['job', 'model', 'unity'],
	}).help().argv;
};

/* eslint-disable no-param-reassign */
const applyDefaultValuesIfUndefined = (config) => {
	config.timeoutMS = config.timeoutMS || 180 * 60 * 1000;
	config.rabbitmq.maxRetries = config.rabbitmq.maxRetries || 3;
	config.rabbitmq.task_prefetch = config.rabbitmq.task_prefetch || 4;
	config.rabbitmq.model_prefetch = config.rabbitmq.model_prefetch || 1;
	config.rabbitmq.unity_prefetch = config.rabbitmq.unity_prefetch || 1;
	config.rabbitmq.waitBeforeShutdownMS = config.rabbitmq.waitBeforeShutdownMS || 60000;
	config.bouncer.log_dir = config.bouncer.log_dir || '.log';
	config.mongoimport = config.mongoimport || {};
	config.mongoimport.writeConcern = config.mongoimport.writeConcern || { w: 1 };
	config.toyModelDir = config.toyModelDir || path.resolve(__dirname, '../../toy');
	config.logLocation = config.logLocation || './bouncer_worker.log';
};
/* eslint-enable no-param-reassign */

// eslint-disable-next-line consistent-return
const init = () => {
	try {
		const params = parseParameters();
		Config.configPath = params.config || path.resolve(__dirname, '../../config.json');
		const config = JSON.parse(fs.readFileSync(Config.configPath));
		config.runOnceOnQueue = params.runOnceOnQueue;
		applyDefaultValuesIfUndefined(config);
		if (config.umask) {
			// can't use logger -> circular dependency.
			// eslint-disable-next-line no-console
			console.log(`Setting umask: ${config.umask}`);
			process.umask(config.umask);
		}
		Config.config = config;
	} catch (err) {
		// can't use logger -> circular dependency.
		// eslint-disable-next-line no-console
		console.error('Failed to parse config file:', err);
		exitApplication(-1);
	}
};

init();

module.exports = Config;
