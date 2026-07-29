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

const assert = require('node:assert');
const { test } = require('node:test');
const ampq = require('amqplib');
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

/*
* This test is currently only run manually with a checkout as CI pipeline is not
* yet configured to run unit tests for bouncer worker.
* This test should be activated when support for bouncer_worker tests in the CI
* is added as part of https://github.com/3drepo/3drepobouncer/issues/768
*/

/*
* This test suite will start and stop its own bouncer, but expects the queue
* to already be running.
*/

// Use the same config as bouncer_worker proper
const { config } = require('../src/lib/config');
const { startBouncerWorker } = require('./helpers');
const { MODEL } = require('../src/constants/queueLabels');
const { PROCESSING } = require('../src/constants/statuses');
const { IMPORT } = require('../src/constants/messageTypes');

const EXIT_TIMEOUT_MS = Math.max(90000, (config.rabbitmq.waitBeforeShutdownMS || 0) + 60000);

test('Test --queue with --exitAfter', { concurrency: false }, async () => {
	const modelq = config.rabbitmq.model_queue;
	const callbackq = config.rabbitmq.callback_queue;
	const connection = await ampq.connect(config.rabbitmq.host);

	const correlationId = crypto.randomUUID().toString();
	const teamspace = 'testTeamspace';
	const container = 'testContainer';

	const importDirectory = path.join(config.rabbitmq.sharedDir, correlationId);
	const importConfigPath = path.join(config.rabbitmq.sharedDir, `${correlationId}.json`);

	const worker = await startBouncerWorker(config, MODEL, 1);

	try {
		fs.mkdirSync(importDirectory, { recursive: true });
		fs.copyFileSync(
			path.join(process.env.REPO_MODEL_PATH, 'cube.obj'),
			path.join(importDirectory, 'cube.obj'),
		);

		const importConfig = {
			lod: 0,
			timezone: 'Europe/London',
			importAnimations: false,
			tag: 'queue_exit_after_test',
			owner: crypto.randomUUID().toString(),
			units: 'm',
			file: `$SHARED_SPACE/${correlationId}/cube.obj`,
			teamspace,
			container,
			revId: crypto.randomUUID().toString(),
		};

		fs.writeFileSync(importConfigPath, JSON.stringify(importConfig));

		// Post one message; --exitAfter=1 should make worker exit after this task.
		{
			const channel = await connection.createChannel();
			await channel.assertQueue(modelq);

			const message = `import -f $SHARED_SPACE/${correlationId}.json`;
			channel.sendToQueue(modelq, Buffer.from(message), { correlationId });
			await channel.close();
		}

		// Wait for processing + completion callbacks.
		{
			const channel = await connection.createChannel();
			await channel.assertQueue(callbackq);
			await channel.prefetch(1);

			await new Promise((resolve, reject) => {
				let seenProcessing = false;

				const timeoutHandle = setTimeout(() => {
					reject(new Error('Timed out waiting for model callbacks in queue/exitAfter test'));
				}, EXIT_TIMEOUT_MS);

				channel.consume(callbackq, (message) => {
					const content = JSON.parse(message.content.toString());

					if (message.properties.correlationId !== correlationId) {
						channel.ack(message);
						return;
					}

					if (content.status === PROCESSING && content.type === MODEL) {
						seenProcessing = true;
						channel.ack(message);
						return;
					}

					assert.equal(content.value, 0);
					assert.equal(content.type, IMPORT);
					assert.equal(content.teamspace, teamspace);
					assert.equal(content.container, container);
					assert.equal(content.user, importConfig.owner);
					assert.equal(seenProcessing, true);

					clearTimeout(timeoutHandle);
					channel.ack(message);
					resolve();
				});
			});

			await channel.close();
		}

		const exitResult = await Promise.race([
			worker.waitForExit(),
			new Promise((resolve) => setTimeout(() => resolve(null), EXIT_TIMEOUT_MS)),
		]);

		assert.notEqual(exitResult, null);
		assert.equal(exitResult.code, 0);
	} finally {
		await worker.stop();
		await connection.close();
	}
});
