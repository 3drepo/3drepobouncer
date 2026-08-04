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

const processMonitorModulePath = require.resolve('../../src/lib/processMonitor');
const siPath = require.resolve('systeminformation');
const fsPath = require.resolve('fs');
const loggerPath = require.resolve('../../src/lib/logger');
const elasticPath = require.resolve('../../src/lib/elastic');
const configPath = require.resolve('../../src/lib/config');
const utilsPath = require.resolve('../../src/lib/utils');

const originalSetInterval = global.setInterval;
const originalClearInterval = global.clearInterval;
const originalDateNow = Date.now;
const originalMathMax = Math.max;

const clearModuleCache = () => {
	delete require.cache[processMonitorModulePath];
	delete require.cache[siPath];
	delete require.cache[fsPath];
	delete require.cache[loggerPath];
	delete require.cache[elasticPath];
	delete require.cache[configPath];
	delete require.cache[utilsPath];
	global.setInterval = originalSetInterval;
	global.clearInterval = originalClearInterval;
	Date.now = originalDateNow;
	Math.max = originalMathMax;
};

const loadProcessMonitorWithMocks = ({
	enabled = true,
	memoryIntervalMS = 10,
	elasticEnabled = true,
	platform = 'win32',
	dockerEnvExists = false,
	memUsed = [200, 280],
	osInfoError,
	memError,
	elasticError,
} = {}) => {
	const logs = {
		info: [],
		error: [],
		verbose: [],
	};
	const calls = {
		createProcessRecord: [],
		sleep: [],
		setInterval: [],
		clearInterval: [],
		existsSync: [],
		readFileSync: [],
		mem: 0,
	};

	let memIndex = 0;
	const nextMemValue = () => {
		const value = memUsed[Math.min(memIndex, memUsed.length - 1)];
		memIndex += 1;
		return value;
	};

	const timerCallbacks = [];
	global.setInterval = (callback, interval) => {
		const token = { token: timerCallbacks.length + 1 };
		timerCallbacks.push(callback);
		calls.setInterval.push(interval);
		return token;
	};
	global.clearInterval = (token) => {
		calls.clearInterval.push(token);
	};

	require.cache[siPath] = {
		id: siPath,
		filename: siPath,
		loaded: true,
		exports: {
			osInfo: async () => {
				if (osInfoError) {
					throw osInfoError;
				}
				return { platform };
			},
			mem: async () => {
				calls.mem += 1;
				if (memError) {
					throw memError;
				}
				return { used: nextMemValue() };
			},
		},
	};

	require.cache[fsPath] = {
		id: fsPath,
		filename: fsPath,
		loaded: true,
		exports: {
			existsSync: (filepath) => {
				calls.existsSync.push(filepath);
				return dockerEnvExists;
			},
			readFileSync: (filepath) => {
				calls.readFileSync.push(filepath);
				return String(nextMemValue());
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

	require.cache[elasticPath] = {
		id: elasticPath,
		filename: elasticPath,
		loaded: true,
		exports: {
			createProcessRecord: async (report) => {
				if (elasticError) {
					throw elasticError;
				}
				calls.createProcessRecord.push(report);
			},
		},
	};

	require.cache[configPath] = {
		id: configPath,
		filename: configPath,
		loaded: true,
		exports: {
			config: {
				processMonitoring: {
					enabled,
					memoryIntervalMS,
				},
				elastic: elasticEnabled,
			},
		},
	};

	require.cache[utilsPath] = {
		id: utilsPath,
		filename: utilsPath,
		loaded: true,
		exports: {
			sleep: async (ms) => {
				calls.sleep.push(ms);
			},
		},
	};

	const ProcessMonitor = moduleRequire(processMonitorModulePath);
	return {
		ProcessMonitor,
		logs,
		calls,
		timerCallbacks,
	};
};

describe(__filename, () => {
	afterEach(clearModuleCache);

	test('startMonitor skips when monitoring is disabled', async () => {
		const { ProcessMonitor, calls, logs } = loadProcessMonitorWithMocks({ enabled: false });

		await ProcessMonitor.startMonitor({ Rid: 'rid-1', Queue: 'modelq' });
		await ProcessMonitor.stopMonitor('rid-1', 0);
		await ProcessMonitor.sendReport('rid-1');

		assert.equal(calls.setInterval.length, 0);
		assert.equal(calls.createProcessRecord.length, 0);
		assert.equal(logs.verbose.length, 0);
	});

	test('start/stop/send tracks memory and emits report to elastic', async () => {
		let now = 100;
		Date.now = () => now;

		const { ProcessMonitor, calls, timerCallbacks, logs } = loadProcessMonitorWithMocks({
			platform: 'win32',
			memUsed: [200, 320],
			memoryIntervalMS: 15,
			elasticEnabled: true,
		});

		const processInfo = { Rid: 'rid-1', Queue: 'modelq', Owner: 'user-1' };
		await ProcessMonitor.startMonitor(processInfo);
		assert.deepEqual(calls.setInterval, [15]);
		assert.equal(timerCallbacks.length, 1);

		await timerCallbacks[0]();
		now = 160;
		await ProcessMonitor.stopMonitor('rid-1', 7);
		await ProcessMonitor.sendReport('rid-1');

		assert.equal(calls.clearInterval.length, 1);
		assert.equal(calls.createProcessRecord.length, 1);
		assert.equal(calls.createProcessRecord[0].Rid, 'rid-1');
		assert.equal(calls.createProcessRecord[0].ReturnCode, 7);
		assert.equal(calls.createProcessRecord[0].MaxMemory, 120);
		assert.equal(calls.createProcessRecord[0].ProcessTime, 60);
		assert.deepEqual(calls.sleep, [15, 15]);
		assert.equal(logs.verbose.some(([message]) => message.includes('Monitoring enabled for revision rid-1')), true);
		assert.equal(logs.verbose.some(([message]) => message.includes('Stopping monitoring for rid-1')), true);
		assert.equal(logs.verbose.some(([message]) => message.includes('Sending report for rid-1')), true);
	});

	test('accumulates previous report data across multiple monitor runs', async () => {
		let now = 100;
		Date.now = () => now;
		const { ProcessMonitor, calls, timerCallbacks } = loadProcessMonitorWithMocks({
			platform: 'win32',
			memUsed: [100, 180, 300, 340],
			memoryIntervalMS: 5,
		});

		await ProcessMonitor.startMonitor({ Rid: 'rid-2', Queue: 'modelq' });
		await timerCallbacks[0]();
		now = 150;
		await ProcessMonitor.stopMonitor('rid-2', 0);

		now = 200;
		await ProcessMonitor.startMonitor({ Rid: 'rid-2', Queue: 'modelq' });
		await timerCallbacks[1]();
		now = 240;
		await ProcessMonitor.stopMonitor('rid-2', 0);
		await ProcessMonitor.sendReport('rid-2');

		assert.equal(calls.createProcessRecord.length, 1);
		assert.equal(calls.createProcessRecord[0].MaxMemory, 80);
		assert.equal(calls.createProcessRecord[0].ProcessTime, 90);
	});

	test('uses docker memory source when running in linux docker', async () => {
		const { ProcessMonitor, calls } = loadProcessMonitorWithMocks({
			platform: 'linux',
			dockerEnvExists: true,
			memUsed: [400, 550],
			memoryIntervalMS: 9,
			elasticEnabled: false,
		});

		await ProcessMonitor.startMonitor({ Rid: 'rid-3', Queue: 'drawingq' });
		await ProcessMonitor.stopMonitor('rid-3', 0);
		await ProcessMonitor.clearReport('rid-3');

		assert.equal(calls.readFileSync.length >= 1, true);
		assert.equal(calls.mem, 0);
		assert.deepEqual(calls.sleep, [9, 9]);
	});

	test('sendReport logs elastic failure and keeps execution stable', async () => {
		const { ProcessMonitor, logs } = loadProcessMonitorWithMocks({
			platform: 'win32',
			memUsed: [111, 120],
			elasticEnabled: true,
			elasticError: new Error('elastic unavailable'),
		});

		await ProcessMonitor.startMonitor({ Rid: 'rid-4', Queue: 'clashq' });
		await ProcessMonitor.stopMonitor('rid-4', 2);
		await ProcessMonitor.sendReport('rid-4');

		assert.equal(logs.error.some(([msg]) => msg.includes('Failed to create elastic record')), true);
	});

	test('handles operating system lookup failure', async () => {
		const { ProcessMonitor, logs, calls } = loadProcessMonitorWithMocks({
			enabled: true,
			osInfoError: new Error('os unavailable'),
		});

		await ProcessMonitor.startMonitor({ Rid: 'rid-5', Queue: 'modelq' });
		await ProcessMonitor.stopMonitor('rid-5', 0);
		await ProcessMonitor.clearReport('rid-5');

		assert.equal(logs.error.some(([msg]) => msg.includes('Failed to get operating system information record')), true);
		assert.equal(calls.setInterval.length, 0);
	});

	test('logs memory read failure when fetching process memory', async () => {
		const { ProcessMonitor, logs } = loadProcessMonitorWithMocks({
			platform: 'win32',
			memError: new Error('mem failed'),
		});

		await ProcessMonitor.startMonitor({ Rid: 'rid-6', Queue: 'modelq' });

		assert.equal(logs.error.some(([msg]) => msg.includes('Failed to get memory information record')), true);
	});

	test('logs updateMemory error when maxMemory update throws', async () => {
		const { ProcessMonitor, timerCallbacks, logs } = loadProcessMonitorWithMocks({
			platform: 'win32',
			memUsed: [100, 150],
		});

		await ProcessMonitor.startMonitor({ Rid: 'rid-7', Queue: 'modelq' });
		Math.max = () => {
			throw new Error('max failed');
		};
		await timerCallbacks[0]();

		assert.equal(logs.error.some(([msg]) => msg.includes('[ProcessMonitor.updateMemory]: Error: max failed')), true);
	});

	test('sendReport logs info when elastic integration is disabled', async () => {
		const { ProcessMonitor, logs } = loadProcessMonitorWithMocks({
			platform: 'win32',
			memUsed: [90, 120],
			elasticEnabled: false,
		});

		await ProcessMonitor.startMonitor({ Rid: 'rid-8', Queue: 'modelq' });
		await ProcessMonitor.stopMonitor('rid-8', 0);
		await ProcessMonitor.sendReport('rid-8');

		assert.equal(logs.info.some(([msg]) => msg.includes('rid-8 stats ProcessTime:')), true);
	});
});
