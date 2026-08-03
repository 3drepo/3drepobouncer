/**
 * Copyright (C) 2026 3D Repo Ltd
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

const { describe, test, afterEach } = require('node:test');
const assert = require('node:assert');
const { createRequire } = require('node:module');
const fs = require('fs');
const os = require('os');
const path = require('path');

const moduleRequire = createRequire(__filename);

const configModulePath = require.resolve('../../src/lib/config');
const processParamsPath = require.resolve('../../src/lib/processParams');
const utilsPath = require.resolve('../../src/lib/utils');

const originalEnvConfig = process.env.BOUNCER_CONFIG;
// eslint-disable-next-line no-console
const originalConsoleError = console.error;
// eslint-disable-next-line no-console
const originalConsoleLog = console.log;
const originalUmask = process.umask;
const originalExistsSync = fs.existsSync;
const originalAccessSync = fs.accessSync;

const cleanup = () => {
	delete require.cache[configModulePath];
	delete require.cache[processParamsPath];
	delete require.cache[utilsPath];
	fs.existsSync = originalExistsSync;
	fs.accessSync = originalAccessSync;
	// eslint-disable-next-line no-console
	console.error = originalConsoleError;
	// eslint-disable-next-line no-console
	console.log = originalConsoleLog;
	process.umask = originalUmask;
	if (originalEnvConfig === undefined) {
		delete process.env.BOUNCER_CONFIG;
	} else {
		process.env.BOUNCER_CONFIG = originalEnvConfig;
	}
};

afterEach(cleanup);

const writeConfig = (tempRoot, data) => {
	const configPath = path.join(tempRoot, 'config.json');
	fs.mkdirSync(path.dirname(configPath), { recursive: true });
	fs.writeFileSync(configPath, JSON.stringify(data));
	return configPath;
};

const buildConfig = ({ sharedDir, taskLogDir, umask, repoLicense, envars, elastic } = {}) => ({
	rabbitmq: {
		host: 'amqp://localhost:5672',
		sharedDir,
	},
	logging: {
		taskLogDir,
		logLevel: 'debug',
		workerLogPath: './worker.log',
	},
	processMonitoring: {
		enabled: false,
		memoryIntervalMS: 250,
	},
	bouncer: {
		path: 'C:/bouncer/client.exe',
		envars,
		log_dir: './logs',
	},
	umask,
	repoLicense,
	elastic,
	timeoutMS: 1000,
	unknownProp: 'ignored',
});

const loadConfig = ({ envConfigPath, paramConfigPath, mockExit, mockError, mockLog } = {}) => {
	if (envConfigPath === undefined) {
		delete process.env.BOUNCER_CONFIG;
	} else {
		process.env.BOUNCER_CONFIG = envConfigPath;
	}

	require.cache[processParamsPath] = {
		id: processParamsPath,
		filename: processParamsPath,
		loaded: true,
		exports: {
			config: paramConfigPath,
		},
	};

	require.cache[utilsPath] = {
		id: utilsPath,
		filename: utilsPath,
		loaded: true,
		exports: {
			exitApplication: mockExit || (() => {}),
		},
	};

	// eslint-disable-next-line no-console
	console.error = mockError || (() => {});
	// eslint-disable-next-line no-console
	console.log = mockLog || (() => {});

	return moduleRequire(configModulePath);
};

describe('config.js', () => {
	test('loads config from BOUNCER_CONFIG and applies defaults/fallbacks', () => {
		const tempRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'bouncer-config-test-'));
		const sharedDir = path.join(tempRoot, 'shared');
		fs.mkdirSync(sharedDir, { recursive: true });

		const configPath = writeConfig(tempRoot, buildConfig({
			sharedDir,
			envars: {
				CUSTOM_FLAG: 'yes',
			},
			elastic: {
				cloud: { id: 'cloud-id' },
				auth: { apiKey: 'api-key' },
				namespace: 'main',
			},
		}));

		const Config = loadConfig({ envConfigPath: configPath });

		assert.equal(Config.configPath, configPath);
		assert.equal(Config.config.logging.taskLogDir, sharedDir);
		assert.equal(Config.config.rabbitmq.callback_queue, 'callbackq');
		assert.equal(Config.config.rabbitmq.model_queue, 'modelq');
		assert.equal(Config.config.rabbitmq.drawing_queue, 'drawingq');
		assert.equal(Config.config.rabbitmq.clash_queue, 'clashq');
		assert.equal(Config.config.rabbitmq.task_prefetch, 4);
		assert.equal(Config.config.rabbitmq.model_prefetch, 1);
		assert.equal(Config.config.rabbitmq.drawing_prefetch, 1);
		assert.equal(Config.config.rabbitmq.pollingIntervalMS, 10000);
		assert.equal(Config.config.rabbitmq.maxRetries, 3);
		assert.equal(Config.config.rabbitmq.waitBeforeShutdownMS, 60000);
		assert.equal(Config.config.rabbitmq.maxWaitTimeMS, 300000);
		assert.equal(Config.config.elastic.maxRetries, 5);
		assert.equal(Config.config.unknownProp, undefined);
		assert.equal(Config.replaceSharedDirTag('$SHARED_SPACE/model.ifc'), `${sharedDir}/model.ifc`);
	});

	test('process args config path takes precedence over BOUNCER_CONFIG', () => {
		const tempRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'bouncer-config-test-'));
		const sharedA = path.join(tempRoot, 'shared-a');
		const sharedB = path.join(tempRoot, 'shared-b');
		fs.mkdirSync(sharedA, { recursive: true });
		fs.mkdirSync(sharedB, { recursive: true });

		const envConfigPath = writeConfig(path.join(tempRoot, 'env'), buildConfig({ sharedDir: sharedA }));
		const paramConfigPath = writeConfig(path.join(tempRoot, 'param'), buildConfig({ sharedDir: sharedB }));

		const Config = loadConfig({
			envConfigPath,
			paramConfigPath,
		});

		assert.equal(Config.configPath, paramConfigPath);
		assert.equal(Config.config.rabbitmq.sharedDir, sharedB);
	});

	test('sets process umask when configured and creates instanceId from repoLicense', () => {
		const tempRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'bouncer-config-test-'));
		const sharedDir = path.join(tempRoot, 'shared');
		fs.mkdirSync(sharedDir, { recursive: true });

		const configPath = writeConfig(tempRoot, buildConfig({
			sharedDir,
			umask: 2,
			repoLicense: 'lic-key',
		}));

		const umaskCalls = [];
		const logs = [];
		process.umask = (value) => {
			umaskCalls.push(value);
			return 0;
		};

		const Config = loadConfig({
			envConfigPath: configPath,
			mockLog: (...args) => logs.push(args.join(' ')),
		});

		assert.deepEqual(umaskCalls, [2]);
		assert.equal(logs.some((entry) => entry.includes('Setting umask: 2')), true);
		assert.equal(Config.config.instanceId, undefined);
	});

	test('fails when default config path is used and file does not exist', () => {
		const exitCodes = [];
		const errorLogs = [];
		const expectedDefaultPath = path.resolve(path.dirname(configModulePath), '../../config.json');

		const Config = loadConfig({
			envConfigPath: undefined,
			paramConfigPath: undefined,
			mockExit: (code) => exitCodes.push(code),
			mockError: (...args) => errorLogs.push(args.join(' ')),
		});

		assert.equal(Config.configPath, expectedDefaultPath);
		assert.deepEqual(exitCodes, [-1]);
		assert.equal(errorLogs.some((entry) => entry.includes('Failed to parse config file:')), true);
	});

	test('fails when shared directory does not exist', () => {
		const tempRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'bouncer-config-test-'));
		const missingDir = path.join(tempRoot, 'missing-shared');
		const configPath = writeConfig(tempRoot, buildConfig({ sharedDir: missingDir }));
		const exitCodes = [];
		const errorLogs = [];

		const Config = loadConfig({
			envConfigPath: configPath,
			mockExit: (code) => exitCodes.push(code),
			mockError: (...args) => errorLogs.push(args.join(' ')),
		});

		assert.equal(Config.configPath, configPath);
		assert.deepEqual(exitCodes, [-1]);
		assert.equal(errorLogs.some((entry) => entry.includes(`Shared directory does not exist: ${missingDir}`)), true);
	});

	test('fails when shared directory is not writable/readable', () => {
		const tempRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'bouncer-config-test-'));
		const sharedDir = path.join(tempRoot, 'shared');
		fs.mkdirSync(sharedDir, { recursive: true });
		const configPath = writeConfig(tempRoot, buildConfig({ sharedDir }));
		const exitCodes = [];
		const errorLogs = [];

		fs.accessSync = () => {
			throw new Error('permission denied');
		};

		const Config = loadConfig({
			envConfigPath: configPath,
			mockExit: (code) => exitCodes.push(code),
			mockError: (...args) => errorLogs.push(args.join(' ')),
		});

		assert.equal(Config.configPath, configPath);
		assert.deepEqual(exitCodes, [-1]);
		assert.equal(errorLogs.some((entry) => entry.includes(`No read access to shared directory: ${sharedDir}`)), true);
	});

	test('fails when config JSON is invalid', () => {
		const tempRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'bouncer-config-test-'));
		const configPath = path.join(tempRoot, 'config.json');
		fs.writeFileSync(configPath, '{invalid json');

		const exitCodes = [];
		const errorLogs = [];

		const Config = loadConfig({
			envConfigPath: configPath,
			mockExit: (code) => exitCodes.push(code),
			mockError: (...args) => errorLogs.push(args.join(' ')),
		});

		assert.equal(Config.configPath, configPath);
		assert.deepEqual(exitCodes, [-1]);
		assert.equal(errorLogs.some((entry) => entry.includes('Failed to parse config file:')), true);
	});
});
