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
const path = require('node:path');
const { createRequire } = require('node:module');

const moduleRequire = createRequire(__filename);

const bouncerClientPath = require.resolve('../../src/tasks/bouncerClient');
const configPath = require.resolve('../../src/lib/config');
const loggerPath = require.resolve('../../src/lib/logger');
const runCommandPath = require.resolve('../../src/lib/runCommand');
const errorCodesPath = require.resolve('../../src/constants/errorCodes');

const { BOUNCER_SOFT_FAILS } = moduleRequire(errorCodesPath);
const expectedBouncerPath = path.normalize('C:/Program Files/3D Repo/Bouncer/bouncer.exe');

const envKeys = ['BC_ENV_ALPHA', 'BC_ENV_BETA', 'REPO_LOG_DIR', 'REPO_LICENSE', 'REPO_INSTANCE_ID'];
const originalEnvValues = {};

envKeys.forEach((key) => {
	originalEnvValues[key] = process.env[key];
});

const clearModuleCache = () => {
	delete require.cache[bouncerClientPath];
	delete require.cache[configPath];
	delete require.cache[loggerPath];
	delete require.cache[runCommandPath];

	envKeys.forEach((key) => {
		if (originalEnvValues[key] === undefined) {
			delete process.env[key];
		} else {
			process.env[key] = originalEnvValues[key];
		}
	});
};

afterEach(clearModuleCache);

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
			path: 'C:/Program Files/3D Repo/Bouncer/bouncer.exe',
			envars: {
				BC_ENV_ALPHA: 'alpha',
				BC_ENV_BETA: 'beta',
			},
		},
		repoLicense: 'repo-lic-123',
		instanceId: 'instance-abc',
	};

	require.cache[configPath] = {
		id: configPath,
		filename: configPath,
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
			configPath: '/tmp/bouncer-config.json',
		},
	};

	require.cache[loggerPath] = {
		id: loggerPath,
		filename: loggerPath,
		loaded: true,
		exports: {
			info: (...args) => calls.loggerInfo.push(args),
			error: (...args) => calls.loggerError.push(args),
		},
	};

	require.cache[runCommandPath] = {
		id: runCommandPath,
		filename: runCommandPath,
		loaded: true,
		exports: async (...args) => {
			calls.run.push(args);
			if (runImpl) {
				return runImpl(...args);
			}
			return 0;
		},
	};

	const bouncerClient = moduleRequire(bouncerClientPath);
	return { bouncerClient, calls };
};

describe('bouncerClient.js', () => {
	test('testClient logs status, sets env and runs bouncer test command', async () => {
		const { bouncerClient, calls } = loadBouncerClientWithMocks();

		await bouncerClient.testClient();

		assert.equal(calls.loggerInfo.some(([msg]) => msg.includes('Checking status of client...')), true);
		assert.equal(calls.loggerInfo.some(([msg]) => msg.includes('Machine Instance ID is set to instance-abc')), true);
		assert.equal(calls.loggerInfo.some(([msg]) => msg.includes('Bouncer call passed')), true);

		assert.equal(calls.run.length, 1);
		assert.equal(calls.run[0][0], expectedBouncerPath);
		assert.deepEqual(calls.run[0][1], ['/tmp/bouncer-config.json', 'test']);
		assert.deepEqual(calls.run[0][2], { logLabel: { label: 'INIT' } });

		assert.equal(process.env.BC_ENV_ALPHA, 'alpha');
		assert.equal(process.env.BC_ENV_BETA, 'beta');
		assert.equal(process.env.REPO_LICENSE, 'repo-lic-123');
		assert.equal(process.env.REPO_INSTANCE_ID, 'instance-abc');
		assert.equal(process.env.REPO_LOG_DIR, undefined);
	});

	test('testClient omits repo-license log and env vars when repo license is not configured', async () => {
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

	test('testClient logs and rethrows run errors', async () => {
		const { bouncerClient, calls } = loadBouncerClientWithMocks({
			runImpl: async () => {
				throw 29;
			},
		});

		await assert.rejects(bouncerClient.testClient(), (err) => err === 29);
		assert.equal(calls.loggerError.some(([msg]) => msg.includes('Bouncer call errored (Error code: 29)')), true);
	});

	test('runBouncerCommand sets log dir and forwards options including soft fail codes', async () => {
		const { bouncerClient, calls } = loadBouncerClientWithMocks({
			runImpl: async () => 7,
		});

		const result = await bouncerClient.runBouncerCommand(
			'/tmp/task-log-dir',
			['import', '/tmp/model.ifc'],
			{ Rid: 'rid-123' },
		);

		assert.equal(result, 7);
		assert.equal(process.env.REPO_LOG_DIR, '/tmp/task-log-dir');
		assert.equal(calls.run.length, 1);
		assert.equal(calls.run[0][0], expectedBouncerPath);
		assert.deepEqual(calls.run[0][1], ['import', '/tmp/model.ifc']);
		assert.deepEqual(calls.run[0][2], {
			codesAsSuccess: BOUNCER_SOFT_FAILS,
			logLabel: { label: 'BOUNCER' },
		});
		assert.deepEqual(calls.run[0][3], { Rid: 'rid-123' });
	});
});
