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

const { randomBytes, randomUUID } = require('node:crypto');
const { spawn } = require('node:child_process');
const fs = require('fs');
const path = require('path');
const { config } = require('../src/lib/config');
const { PROCESSING } = require('../src/constants/statuses');

const FORCE_KILL_TIMEOUT_MS = 10000;
const WORKER_LOG_LINES = 40;

const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

const generateUUIDString = () => randomUUID().toString();

const generateRandomString = (length = 8) => randomBytes(length).toString('hex');

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

const createCorrelationIdAndSharedDirectory = (filesToCopy) => {
	const correlationId = generateUUIDString();
	const correlationDirectory = path.join(config.rabbitmq.sharedDir, correlationId);
	if (filesToCopy?.length) {
		fs.mkdirSync(correlationDirectory, { recursive: true });
		for (const file of filesToCopy) {
			fs.copyFileSync(
				file,
				path.join(correlationDirectory, path.basename(file)),
			);
		}
	}
	return correlationId;
};

const postToQueue = async (connection, queueName, message, correlationId) => {
	const channel = await connection.createChannel();
	await channel.assertQueue(queueName);
	channel.sendToQueue(queueName, Buffer.from(message), { correlationId });
	// Closing the channel flushes the message
	await channel.close();
};

const waitForCallback = async (connection, callbackq, correlationId, { onProcessing, onComplete, timeoutMessage }) => {
	const channel = await connection.createChannel();
	await channel.assertQueue(callbackq);
	await channel.prefetch(1);

	try {
		await new Promise((resolve, reject) => {
			let seenProcessing = false;

			const timeoutHandle = setTimeout(() => {
				reject(new Error(timeoutMessage));
			}, 60000);

			channel.consume(callbackq, (message) => {
				const content = JSON.parse(message.content.toString());

				if (message.properties.correlationId !== correlationId) {
					channel.ack(message);
					return;
				}

				try {
					if (content.status === PROCESSING) {
						onProcessing(content);
						seenProcessing = true;
						channel.ack(message);
						return;
					}

					onComplete(content, seenProcessing);
				} catch (err) {
					clearTimeout(timeoutHandle);
					channel.ack(message);
					reject(err);
					return;
				}

				clearTimeout(timeoutHandle);
				channel.ack(message);
				resolve();
			});
		});
	} finally {
		await channel.close();
	}
};

// eslint-disable-next-line max-len
module.exports = { createCorrelationIdAndSharedDirectory, generateRandomString, generateUUIDString, startBouncerWorker, postToQueue, waitForCallback };
