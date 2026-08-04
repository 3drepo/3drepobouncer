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

const moduleRequire = createRequire(__filename);

const loggerModulePath = require.resolve('../../src/lib/logger');
const configPath = require.resolve('../../src/lib/config');
const winstonPath = require.resolve('winston');

const clearModuleCache = () => {
	delete require.cache[loggerModulePath];
	delete require.cache[configPath];
	delete require.cache[winstonPath];
};

const loadLoggerWithMocks = ({
	logLevel = 'info',
	noColors = false,
	jsonOutput = false,
	workerLogPath,
} = {}) => {
	const calls = {
		createLogger: [],
		consoleTransport: [],
		fileTransport: [],
		combine: [],
		printf: [],
	};

	const makeToken = (name) => ({ token: name });

	const mockWinston = {
		transports: {
			Console: function Console(options) {
				calls.consoleTransport.push(options);
				this.kind = 'console';
				this.options = options;
			},
			File: function File(options) {
				calls.fileTransport.push(options);
				this.kind = 'file';
				this.options = options;
			},
		},
		format: {
			timestamp: () => makeToken('timestamp'),
			align: () => makeToken('align'),
			json: () => makeToken('json'),
			colorize: () => makeToken('colorize'),
			printf: (formatter) => {
				calls.printf.push(formatter);
				return { token: 'printf', formatter };
			},
			combine: (...parts) => {
				calls.combine.push(parts);
				return { token: 'combine', parts };
			},
		},
		createLogger: (options) => {
			calls.createLogger.push(options);
			return {
				__options: options,
				info: () => {},
				error: () => {},
				verbose: () => {},
			};
		},
	};

	require.cache[winstonPath] = {
		id: winstonPath,
		filename: winstonPath,
		loaded: true,
		exports: mockWinston,
	};

	require.cache[configPath] = {
		id: configPath,
		filename: configPath,
		loaded: true,
		exports: {
			config: {
				logging: {
					logLevel,
					noColors,
					jsonOutput,
					workerLogPath,
				},
			},
		},
	};

	const logger = moduleRequire(loggerModulePath);
	return { logger, calls };
};

describe(__filename, () => {
	afterEach(clearModuleCache);

	test('uses colorized text formatter by default and console transport', () => {
		const { logger, calls } = loadLoggerWithMocks({
			logLevel: 'verbose',
			noColors: false,
			jsonOutput: false,
		});

		assert.equal(typeof logger.info, 'function');
		assert.equal(calls.consoleTransport.length, 1);
		assert.equal(calls.fileTransport.length, 0);
		assert.equal(calls.consoleTransport[0].level, 'verbose');
		assert.equal(calls.createLogger.length, 1);

		const formatParts = calls.combine[0].map((part) => part.token);
		assert.deepEqual(formatParts, ['timestamp', 'colorize', 'align', 'printf']);
		assert.equal(calls.printf.length, 1);

		const rendered = calls.printf[0]({
			level: 'info',
			message: 'hello world',
			timestamp: '2026-08-03T12:00:00.000Z',
		});
		assert.equal(rendered, '2026-08-03T12:00:00.000Z [info] [APP] hello world');
	});

	test('uses plain text formatter without colors and adds file transport when configured', () => {
		const { calls } = loadLoggerWithMocks({
			logLevel: 'debug',
			noColors: true,
			jsonOutput: false,
			workerLogPath: './log/worker.log',
		});

		assert.equal(calls.consoleTransport.length, 1);
		assert.equal(calls.fileTransport.length, 1);
		assert.equal(calls.fileTransport[0].filename, './log/worker.log');
		assert.equal(calls.fileTransport[0].level, 'debug');

		const formatParts = calls.combine[0].map((part) => part.token);
		assert.deepEqual(formatParts, ['timestamp', 'align', 'printf']);
		assert.equal(formatParts.includes('colorize'), false);

		const rendered = calls.printf[0]({
			level: 'warn',
			message: 'disk is full',
			label: 'AMQP',
			timestamp: '2026-08-03T12:01:00.000Z',
		});
		assert.equal(rendered, '2026-08-03T12:01:00.000Z [warn] [AMQP] disk is full');
	});

	test('uses json formatter pipeline when jsonOutput is enabled', () => {
		const { calls } = loadLoggerWithMocks({
			logLevel: 'info',
			jsonOutput: true,
			noColors: false,
			workerLogPath: './log/worker.log',
		});

		assert.equal(calls.consoleTransport.length, 1);
		assert.equal(calls.fileTransport.length, 1);
		assert.equal(calls.printf.length, 0);

		const formatParts = calls.combine[0].map((part) => part.token);
		assert.deepEqual(formatParts, ['timestamp', 'align', 'json']);
	});
});
