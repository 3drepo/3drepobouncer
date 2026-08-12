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
const { generateRandomString, generateRandomFilepath, generateRandomSentence } = require('../random');

const moduleRequire = createRequire(__filename);

const runCommandPath = require.resolve('../../src/lib/runCommand');
const childProcessPath = require.resolve('child_process');
const killPath = require.resolve('tree-kill');
const loggerPath = require.resolve('../../src/lib/logger');
const processMonitorPath = require.resolve('../../src/lib/processMonitor');
const configPath = require.resolve('../../src/lib/config');
const errorCodesPath = require.resolve('../../src/constants/errorCodes');

const { ERRCODE_TIMEOUT, ERRCODE_UNKNOWN_ERROR } = moduleRequire(errorCodesPath);

const originalSetTimeout = global.setTimeout;

const clearModuleCache = () => {
	delete require.cache[runCommandPath];
	delete require.cache[childProcessPath];
	delete require.cache[killPath];
	delete require.cache[loggerPath];
	delete require.cache[processMonitorPath];
	delete require.cache[configPath];
	global.setTimeout = originalSetTimeout;
};

const createMockChild = () => {
	const closeHandlers = [];
	const stdoutHandlers = [];
	const stderrHandlers = [];

	return {
		pid: 321,
		on: (event, handler) => {
			if (event === 'close') {
				closeHandlers.push(handler);
			}
		},
		stdout: {
			on: (event, handler) => {
				if (event === 'data') {
					stdoutHandlers.push(handler);
				}
			},
		},
		stderr: {
			on: (event, handler) => {
				if (event === 'data') {
					stderrHandlers.push(handler);
				}
			},
		},
		emitClose: (code, signal) => closeHandlers.forEach((handler) => handler(code, signal)),
		emitStdout: (data) => stdoutHandlers.forEach((handler) => handler(data)),
		emitStderr: (data) => stderrHandlers.forEach((handler) => handler(data)),
	};
};

const loadRunCommandWithMocks = ({ timeoutMS = 5000 } = {}) => {
	const calls = {
		spawn: [],
		kill: [],
		loggerInfo: [],
		loggerVerbose: [],
		startMonitor: [],
		stopMonitor: [],
		timeouts: [],
	};

	const child = createMockChild();
	let timeoutCallback;

	global.setTimeout = (callback, ms) => {
		calls.timeouts.push(ms);
		timeoutCallback = callback;
		return 1;
	};

	require.cache[childProcessPath] = {
		id: childProcessPath,
		filename: childProcessPath,
		loaded: true,
		exports: {
			spawn: (exe, params, options) => {
				calls.spawn.push({ exe, params, options });
				return child;
			},
		},
	};

	require.cache[killPath] = {
		id: killPath,
		filename: killPath,
		loaded: true,
		exports: (pid) => calls.kill.push(pid),
	};

	require.cache[loggerPath] = {
		id: loggerPath,
		filename: loggerPath,
		loaded: true,
		exports: {
			info: (...args) => calls.loggerInfo.push(args),
			verbose: (...args) => calls.loggerVerbose.push(args),
		},
	};

	require.cache[processMonitorPath] = {
		id: processMonitorPath,
		filename: processMonitorPath,
		loaded: true,
		exports: {
			startMonitor: (info) => calls.startMonitor.push(info),
			stopMonitor: (rid, code) => calls.stopMonitor.push({ rid, code }),
		},
	};

	require.cache[configPath] = {
		id: configPath,
		filename: configPath,
		loaded: true,
		exports: {
			config: {
				timeoutMS,
			},
		},
	};

	const runCommand = moduleRequire(runCommandPath);
	return { runCommand, child, calls, triggerTimeout: () => timeoutCallback() };
};

describe(__filename, () => {
	afterEach(clearModuleCache);

	test('resolves on zero exit code and logs stdout/stderr when verbose', async () => {
		const { runCommand, child, calls } = loadRunCommandWithMocks({ timeoutMS: 2000 });
		const Rid = generateRandomString();
		const processInfo = { Rid };
		const logLabel = { label: 'INIT' };
		const filename = generateRandomFilepath();
		const tool = generateRandomFilepath();

		const resultPromise = runCommand(tool, ['-f', filename], { logLabel }, processInfo);

		const stdout = generateRandomSentence();
		const stderr = generateRandomSentence();

		child.emitStdout(Buffer.from(stdout));
		child.emitStderr(Buffer.from(stderr));
		child.emitClose(0, null);
		const result = await resultPromise;

		assert.equal(result, 0);
		assert.equal(calls.spawn.length, 1);
		assert.equal(calls.spawn[0].exe, `"${tool}"`);
		assert.deepEqual(calls.spawn[0].params, ['-f', filename]);
		assert.equal(calls.spawn[0].options.shell, true);
		assert.deepEqual(calls.startMonitor, [processInfo]);
		assert.deepEqual(calls.stopMonitor, [{ rid: Rid, code: 0 }]);
		assert.deepEqual(calls.timeouts, [2000]);
		assert.equal(calls.loggerInfo.some(([msg]) => msg.includes(`Executing command: "${tool}" -f ${filename}`)), true);
		assert.equal(calls.loggerInfo.some(([msg]) => msg.includes('Command executed. Code: 0 signal: null')), true);
		assert.equal(calls.loggerVerbose.some(([msg]) => msg.includes(`[STDOUT]: ${stdout}`)), true);
		assert.equal(calls.loggerVerbose.some(([msg]) => msg.includes(`[STDERR]: ${stderr}`)), true);
	});

	test('resolves when exit code is marked as success', async () => {
		const { runCommand, child } = loadRunCommandWithMocks();
		const tool = generateRandomFilepath();
		const resultPromise = runCommand(`${tool}`, [], { codesAsSuccess: [7], logLabel: { label: 'INIT' } });
		child.emitClose(7, null);
		const result = await resultPromise;
		assert.equal(result, 7);
	});

	test('rejects with exit code on hard failure', async () => {
		const { runCommand, child, calls } = loadRunCommandWithMocks();
		const tool = generateRandomFilepath();
		const resultPromise = runCommand(`${tool}`, [], { logLabel: { label: 'INIT' } });
		const code = Math.floor(Math.random() * 10) + 2;
		child.emitClose(code, null);
		await assert.rejects(resultPromise, (err) => err === code);
		assert.equal(calls.loggerInfo.some(([msg]) => msg.includes(`exiting with ERRCODE_UNKNOWN_ERROR: ${code} signal: null`)), true);
	});

	test('rejects with ERRCODE_UNKNOWN_ERROR when close code is null', async () => {
		const { runCommand, child } = loadRunCommandWithMocks();
		const tool = generateRandomFilepath();
		const resultPromise = runCommand(`${tool}`, [], { logLabel: { label: 'INIT' } });
		child.emitClose(null, 'SIGTERM');
		await assert.rejects(resultPromise, (err) => err === ERRCODE_UNKNOWN_ERROR);
	});

	test('rejects with ERRCODE_TIMEOUT and kills process when timeout elapses first', async () => {
		const { runCommand, child, calls, triggerTimeout } = loadRunCommandWithMocks();
		const tool = generateRandomFilepath();
		const Rid = generateRandomString();
		const processInfo = { Rid };
		const resultPromise = runCommand(`${tool}`, [], { logLabel: { label: 'INIT' } }, processInfo);

		triggerTimeout();
		child.emitClose(null, 'SIGKILL');

		await assert.rejects(resultPromise, (err) => err === ERRCODE_TIMEOUT);
		assert.deepEqual(calls.kill, [321]);
		assert.deepEqual(calls.stopMonitor, [{ rid: Rid, code: ERRCODE_TIMEOUT }]);
		assert.equal(calls.loggerInfo.some(([msg]) => msg.includes('Max processing time reached, terminating the process')), true);
		assert.equal(calls.loggerInfo.some(([msg]) => msg.includes('Command executed. Code: TIMEDOUT signal: SIGKILL')), true);
	});

	test('does not log execution line when verbose is false', async () => {
		const { runCommand, child, calls } = loadRunCommandWithMocks();
		const tool = generateRandomFilepath();
		const resultPromise = runCommand(`${tool}`, [], { verbose: false, logLabel: { label: 'INIT' } });
		child.emitClose(0, null);
		await resultPromise;
		assert.equal(calls.loggerInfo.some(([msg]) => msg.startsWith('Executing command:')), false);
	});
});
