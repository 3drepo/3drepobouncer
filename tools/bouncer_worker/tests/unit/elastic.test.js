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
const { generateRandomString } = require('../random');

const moduleRequire = createRequire(__filename);

const elasticModulePath = require.resolve('../../src/lib/elastic');
const elasticPackagePath = require.resolve('@elastic/elasticsearch');
const configPath = require.resolve('../../src/lib/config');
const loggerPath = require.resolve('../../src/lib/logger');
const utilsPath = require.resolve('../../src/lib/utils');

const clearModuleCache = () => {
	delete require.cache[elasticModulePath];
	delete require.cache[elasticPackagePath];
	delete require.cache[configPath];
	delete require.cache[loggerPath];
	delete require.cache[utilsPath];
};

const loadElasticWithMocks = ({ elasticConfig, indexExists = false, healthError, createError } = {}) => {
	const logs = {
		info: [],
		error: [],
		verbose: [],
	};
	const exitCalls = [];
	const calls = {
		constructorArgs: [],
		health: 0,
		exists: [],
		createIndex: [],
		putMapping: [],
		createDoc: [],
	};

	class MockElasticClient {
		constructor(args) {
			calls.constructorArgs.push(args);
			this.create = async (payload) => {
				if (createError) {
					throw createError;
				}
				calls.createDoc.push(payload);
			};
			this.cluster = {
				health: async () => {
					calls.health += 1;
					if (healthError) {
						throw healthError;
					}
				},
			};
			this.indices = {
				exists: async ({ index }) => {
					calls.exists.push(index);
					return { body: indexExists };
				},
				create: async ({ index }) => {
					calls.createIndex.push(index);
				},
				putMapping: async ({ index, body }) => {
					calls.putMapping.push({ index, body });
				},
			};
		}
	}

	require.cache[elasticPackagePath] = {
		id: elasticPackagePath,
		filename: elasticPackagePath,
		loaded: true,
		exports: {
			Client: MockElasticClient,
		},
	};

	require.cache[configPath] = {
		id: configPath,
		filename: configPath,
		loaded: true,
		exports: {
			config: {
				elastic: elasticConfig,
			},
		},
	};

	require.cache[loggerPath] = {
		id: loggerPath,
		filename: loggerPath,
		loaded: true,
		exports: {
			info: (...args) => logs.info.push(args),
			error: (...args) => logs.error.push(args),
			verbose: (...args) => logs.verbose.push(args),
		},
	};

	const hashedId = generateRandomString();

	require.cache[utilsPath] = {
		id: utilsPath,
		filename: utilsPath,
		loaded: true,
		exports: {
			exitApplication: () => exitCalls.push(true),
			hashCode: () => hashedId,
		},
	};

	const Elastic = moduleRequire(elasticModulePath);
	return { Elastic, logs, exitCalls, calls, hashedId };
};

describe(__filename, () => {
	afterEach(clearModuleCache);

	test('returns early when elastic config is not defined', async () => {
		const { Elastic, calls } = loadElasticWithMocks({ elasticConfig: undefined });

		await Elastic.createProcessRecord({ Owner: generateRandomString() });

		assert.equal(calls.constructorArgs.length, 0);
		assert.equal(calls.createDoc.length, 0);
	});

	test('initializes elastic client, creates index/mapping and writes process record', async () => {
		const elasticConfig = {
			cloud: { id: generateRandomString() },
			auth: { apiKey: generateRandomString() },
			namespace: generateRandomString(),
		};
		const { Elastic, calls, logs, hashedId } = loadElasticWithMocks({ elasticConfig, indexExists: false });

		const payload = {
			Owner: generateRandomString(),
			Model: generateRandomString(),
			Database: generateRandomString(),
		};

		await Elastic.createProcessRecord(payload);

		assert.equal(calls.constructorArgs.length, 1);
		assert.deepEqual(calls.constructorArgs[0], elasticConfig);
		assert.equal(calls.health, 1);
		assert.deepEqual(calls.exists, ['io-bouncer']);
		assert.deepEqual(calls.createIndex, ['io-bouncer']);
		assert.equal(calls.putMapping.length, 1);
		assert.equal(calls.putMapping[0].index, 'io-bouncer');
		assert.equal(calls.putMapping[0].body.properties.Owner.type, 'keyword');
		assert.equal(calls.createDoc.length, 1);
		assert.equal(calls.createDoc[0].index, 'io-bouncer');
		assert.equal(calls.createDoc[0].id, hashedId);
		assert.equal(calls.createDoc[0].refresh, true);
		assert.equal(calls.createDoc[0].body.namespace, elasticConfig.namespace);
		assert.equal(logs.info.some(([msg]) => msg.includes(`connected to ${elasticConfig.cloud.id}`)), true);
		assert.equal(logs.info.some(([msg]) => msg.includes('Created index io-bouncer')), true);
		assert.equal(logs.info.some(([msg]) => msg.includes('Created mapping io-bouncer')), true);
	});

	test('does not create index or mapping when index already exists', async () => {
		const elasticConfig = {
			cloud: { id: generateRandomString() },
			auth: { apiKey: generateRandomString() },
			namespace: generateRandomString(),
		};
		const { Elastic, calls } = loadElasticWithMocks({ elasticConfig, indexExists: true });

		await Elastic.createProcessRecord({ Owner: generateRandomString() });

		assert.equal(calls.createIndex.length, 0);
		assert.equal(calls.putMapping.length, 0);
	});

	test('logs health check failure and calls exitApplication', async () => {
		const elasticConfig = {
			cloud: { id: generateRandomString() },
			auth: { apiKey: generateRandomString() },
			namespace: generateRandomString(),
		};
		const { Elastic, logs, exitCalls } = loadElasticWithMocks({
			elasticConfig,
			healthError: new Error('health failed'),
			indexExists: true,
		});

		await Elastic.createProcessRecord({ Owner: generateRandomString() });

		assert.equal(exitCalls.length, 1);
		assert.equal(logs.error.some(([msg]) => msg.includes('Health check failed on elastic connection')), true);
	});

	test('logs create failures', async () => {
		const elasticConfig = {
			cloud: { id: generateRandomString() },
			auth: { apiKey: generateRandomString() },
			namespace: generateRandomString(),
		};
		const { Elastic, logs } = loadElasticWithMocks({
			elasticConfig,
			createError: new Error('create failed'),
			indexExists: true,
		});

		await Elastic.createProcessRecord({ Owner: generateRandomString() });

		assert.equal(logs.error.some(([msg]) => msg.includes('createElasticRecord Error: create failed io-bouncer')), true);
	});

	test('does not create record when payload is empty', async () => {
		const elasticConfig = {
			cloud: { id: generateRandomString() },
			auth: { apiKey: generateRandomString() },
			namespace: generateRandomString(),
		};
		const { Elastic, calls } = loadElasticWithMocks({ elasticConfig, indexExists: true });

		await Elastic.createProcessRecord(undefined);

		assert.equal(calls.createDoc.length, 0);
	});
});
