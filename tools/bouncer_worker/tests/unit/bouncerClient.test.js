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
const { generateRandomString, generateRandomPath } = require('../random');

const moduleRequire = createRequire(__filename);

const bouncerClientModulePath = require.resolve('../../src/tasks/bouncerClient');
const configModulePath = require.resolve('../../src/lib/config');
const loggerModulePath = require.resolve('../../src/lib/logger');
const runCommandModulePath = require.resolve('../../src/lib/runCommand');
const errorCodesModulePath = require.resolve('../../src/constants/errorCodes');

const { BOUNCER_SOFT_FAILS } = moduleRequire(errorCodesModulePath);

const envKeys = ['BC_ENV_ALPHA', 'BC_ENV_BETA', 'REPO_LOG_DIR', 'REPO_LICENSE', 'REPO_INSTANCE_ID'];
const originalEnvValues = {};

envKeys.forEach((key) => {
	originalEnvValues[key] = process.env[key];
});

const clearModuleCache = () => {
	delete require.cache[bouncerClientModulePath];
	delete require.cache[configModulePath];
	delete require.cache[loggerModulePath];
	delete require.cache[runCommandModulePath];

	envKeys.forEach((key) => {
		if (originalEnvValues[key] === undefined) {
			delete process.env[key];
		} else {
			process.env[key] = originalEnvValues[key];
		}
	});
};

const loadBouncerClientWithMocks = ({
	runImpl,
	configOverrides,
} = {}) => {
	const calls = {
		run: [],
		loggerInfo: [],
		loggerError: [],
	};

	const baseConfig = {
		bouncer: {
			path: generateRandomPath(),
			envars: {
				BC_ENV_ALPHA: generateRandomString(),
				BC_ENV_BETA: generateRandomString(),
			},
		},
		repoLicense: generateRandomString(),
		instanceId: generateRandomString(),
	};

	require.cache[configModulePath] = {
		id: configModulePath,
		filename: configModulePath,
		loaded: true,
		exports: {
			config: {
				...baseConfig,
				...configOverrides,
				bouncer: {
					...baseConfig.bouncer,
					...(configOverrides && configOverrides.bouncer),
				},
			},
			configPath: generateRandomString(),
		},
	};

	require.cache[loggerModulePath] = {
		id: loggerModulePath,
		filename: loggerModulePath,
		loaded: true,
		exports: {
			info: (...args) => calls.loggerInfo.push(args),
			error: (...args) => calls.loggerError.push(args),
		},
	};

	require.cache[runCommandModulePath] = {
		id: runCommandModulePath,
		filename: runCommandModulePath,
		loaded: true,
		exports: async (...args) => {
			calls.run.push(args);
			if (runImpl) {
				return runImpl(...args);
			}
			return 0;
		},
	};

	const bouncerClient = moduleRequire(bouncerClientModulePath);
	return {
		bouncerClient,
		calls,
		config: require.cache[configModulePath].exports.config,
		configPath: require.cache[configModulePath].exports.configPath,
	};
};

const testTestClient = () => {
	describe('testClient', () => {
		afterEach(clearModuleCache);

		test('logs status, sets env and runs bouncer test command', async () => {
			const { bouncerClient, calls, config, configPath } = loadBouncerClientWithMocks();

			await bouncerClient.testClient();

			assert.equal(calls.loggerInfo.some(([msg]) => msg.includes('Checking status of client...')), true);
			assert.equal(calls.loggerInfo.some(([msg]) => msg.includes(`Machine Instance ID is set to ${config.instanceId}`)), true);
			assert.equal(calls.loggerInfo.some(([msg]) => msg.includes('Bouncer call passed')), true);

			assert.equal(calls.run.length, 1);
			assert.equal(calls.run[0][0], config.bouncer.path);
			assert.deepEqual(calls.run[0][1], [configPath, 'test']);
			assert.deepEqual(calls.run[0][2], { logLabel: { label: 'INIT' } });

			assert.equal(process.env.BC_ENV_ALPHA, config.bouncer.envars.BC_ENV_ALPHA);
			assert.equal(process.env.BC_ENV_BETA, config.bouncer.envars.BC_ENV_BETA);
			assert.equal(process.env.REPO_LICENSE, config.repoLicense);
			assert.equal(process.env.REPO_INSTANCE_ID, config.instanceId);
			assert.equal(process.env.REPO_LOG_DIR, originalEnvValues.REPO_LOG_DIR);
		});

		test('omits repo-license log and env vars when repo license is not configured', async () => {
			const { bouncerClient, calls } = loadBouncerClientWithMocks({
				configOverrides: {
					repoLicense: undefined,
					instanceId: undefined,
				},
			});

			await bouncerClient.testClient();

			assert.equal(calls.loggerInfo.some(([msg]) => msg.includes('Machine Instance ID is set to')), false);
			assert.equal(process.env.REPO_LICENSE, undefined);
			assert.equal(process.env.REPO_INSTANCE_ID, undefined);
		});

		test('logs and rethrows run errors', async () => {
			const { bouncerClient, calls } = loadBouncerClientWithMocks({
				runImpl: async () => {
					throw 29;
				},
			});

			await assert.rejects(bouncerClient.testClient(), (err) => err === 29);
			assert.equal(calls.loggerError.some(([msg]) => msg.includes('Bouncer call errored (Error code: 29)')), true);
		});
	});
};

const testRunBouncerCommand = () => {
	describe('RunBouncerCommand', () => {
		afterEach(clearModuleCache);

		test('sets log dir and forwards options including soft fail codes', async () => {
			const { bouncerClient, calls, config } = loadBouncerClientWithMocks({
				runImpl: async () => 7,
			});

			const taskLogDir = generateRandomPath();
			const modelDir = generateRandomPath();
			const rid = generateRandomString();

			const result = await bouncerClient.runBouncerCommand(
				taskLogDir,
				['import', modelDir],
				{ Rid: rid },
			);

			assert.equal(result, 7);
			assert.equal(process.env.REPO_LOG_DIR, taskLogDir);
			assert.equal(calls.run.length, 1);
			assert.equal(calls.run[0][0], config.bouncer.path);
			assert.deepEqual(calls.run[0][1], ['import', modelDir]);
			assert.deepEqual(calls.run[0][2], {
				codesAsSuccess: BOUNCER_SOFT_FAILS,
				logLabel: { label: 'BOUNCER' },
			});
			assert.deepEqual(calls.run[0][3], { Rid: rid });
		});
	});
};

describe(__filename, () => {
	testTestClient();
	testRunBouncerCommand();
});
