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
const { v4: uuidv4 } = require('uuid');
const { object, string, number, lazy } = require('yup');
const { exitApplication } = require('./utils');
const params = require('./processParams');

const Config = {};

const rabbitmq = object({
	host: string().required(),
	sharedDir: string().required(),
	worker_queue: string().default('jobq'),
	callback_queue: string().default('callbackq'),
	model_queue: string().default('modelq'),
	drawing_queue: string().default('drawingq'),
	task_prefetch: number().default(4),
	model_prefetch: number().default(1),
	drawing_prefetch: number().default(1),
	pollingIntervalMS: number().default(10 * 1000),
	maxRetries: number().default(3),
	waitBeforeShutdownMS: number().default(60000),
	maxWaitTimeMS: number().default(5 * 60 * 1000),
});

// This schema considers an object as a set of key-value pairs, where each
// value should be a string.
const envars = lazy((kvpObject) => {
	const kvpSchema = {};
	for (const key of Object.keys(kvpObject)) {
		kvpSchema[key] = string();
	}
	return object(kvpSchema);
});

const bouncer = object({
	path: string().required(),
	envars,
	log_dir: string(),
});

const logging = object({
	taskLogDir: string(),
	workerLogPath: string(),
	logLevel: string().oneOf([ // Options are the six winston log levels https://www.npmjs.com/package/winston/v/2.4.6#logging-levels
		'error',
		'warn',
		'info',
		'verbose',
		'debug',
		'silly',
	]).default('info'),
});

const processMonitoring = object({
	memoryIntervalMS: number().default(100),
});

const schema = object({
	timeoutMS: number().default(180 * 60 * 1000),
	umask: number(),
	repoLicense: string(),
	instanceId: string().when('repoLicense', { // This entry ensures that if repoLicense is defined, so is the instanceId
		is: true,
		then: (sch) => sch.default(uuidv4()),
	}),
	rabbitmq,
	logging,
	processMonitoring,
	bouncer,
});

// eslint-disable-next-line consistent-return
const init = () => {
	try {
		Config.configPath = params.config || path.resolve(__dirname, '../../config.json');
		let config = JSON.parse(fs.readFileSync(Config.configPath));
		config = schema.validateSync(config, {
			stripUnknown: true,
		});

		// Set the fallbacks for log file directories (all log directories are
		// ultimately optional).
		if (!config.taskLogDir) {
			config.taskLogDir = config.rabbitmq.sharedDir;
		}

		// Set the file creation permission mode mask (i.e. what permissions are
		// *not* set) for any files created by this process or its descendents.
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
