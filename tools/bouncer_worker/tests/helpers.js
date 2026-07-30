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

const { spawn } = require('node:child_process');

const FORCE_KILL_TIMEOUT_MS = 10000;
const WORKER_LOG_LINES = 40;

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

/**
 * Starts a bouncer worker instance and waits until it is ready to consume
 * messages from all configured queues.
 *
 * @returns {Promise<{ stop: () => Promise<{ code: number|null, signal: NodeJS.Signals|null }>, waitForExit: () => Promise<{ code: number|null, signal: NodeJS.Signals|null }> }>} A handle with stop() and waitForExit() methods.
 */
const startBouncerWorker = async (queue = undefined, exitAfter = undefined) => {
	const output = [];

	const args = [];

	if (queue) {
		args.push('--queue');
		args.push(queue);
	}

	if (Number.isInteger(exitAfter) && exitAfter > 0) {
		args.push('--exitAfter');
		args.push(exitAfter.toString());
	}

	const worker = spawn(process.execPath, [
		'src/scripts/bouncer_worker.js',
		...args,
	], {
		cwd: process.cwd(),
		env: process.env,
		stdio: ['ignore', 'pipe', 'pipe'],
	});

	const collect = (chunk) => {
		output.push(chunk.toString());
		if (output.length > WORKER_LOG_LINES) {
			output.splice(0, output.length - WORKER_LOG_LINES);
		}
	};

	worker.stdout.on('data', collect);
	worker.stderr.on('data', collect);

	if (process.env.BOUNCER_CONSOLE_OUT === '1') {
		worker.stdout.pipe(process.stdout);
		worker.stderr.pipe(process.stderr);
	}

	const workerExitPromise = new Promise((resolve) => {
		worker.once('exit', (code, signal) => resolve({ code, signal, output }));
	});

	const stop = async () => {
		if (!worker || worker.exitCode !== null || worker.killed) {
			return workerExitPromise;
		}

		worker.kill('SIGTERM');

		const gracefulExitResult = await Promise.race([
			workerExitPromise,
			sleep(FORCE_KILL_TIMEOUT_MS).then(() => null),
		]);

		if (gracefulExitResult) {
			return gracefulExitResult;
		}

		worker.kill('SIGKILL');
		return workerExitPromise;
	};

	const waitForExit = () => workerExitPromise;

	return { stop, waitForExit };
};

module.exports = { startBouncerWorker };
