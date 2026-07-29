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
const { before, after, test } = require('node:test');
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
const { config, replaceSharedDirTag } = require('../src/lib/config');
const { startBouncerWorker } = require('./helpers');
const { CLASH } = require('../src/constants/queueLabels');
const { PROCESSING } = require('../src/constants/statuses');
const { CLASH: CLASH_TYPE } = require('../src/constants/messageTypes');

let stopBouncerWorker = null;

before(async () => {
	const { stop } = await startBouncerWorker(config, CLASH);
	stopBouncerWorker = stop;
});

after(async () => {
	await stopBouncerWorker();
});

test('Test Clash Q', { concurrency: true }, async () => {
	const clashq = config.rabbitmq.clash_queue;
	const callbackq = config.rabbitmq.callback_queue;

	const connection = await ampq.connect(config.rabbitmq.host);

	const correlationId = crypto.randomUUID().toString();

	// In practice, these are used to determine where to look for the clash run
	// in the database. They are completely independent of which geometries
	// are clashed, so can be anything here.

	const project = 'testProject';
	const teamspace = 'testTeamspace';

	const clashConfigDirectory = path.join(config.rabbitmq.sharedDir, correlationId);

	// This config in the tests repo is a valid config that performs a clash
	// using one of the ClashDetection containers in the database dump. We
	// copy it to $SHARED_SPACE in order to test the shared space tag
	// substitution logic as well.

	fs.mkdirSync(clashConfigDirectory, { recursive: true });
	fs.copyFileSync(
		path.join(process.env.REPO_MODEL_PATH, 'clash/simple_clash_config.json'),
		path.join(clashConfigDirectory, 'clashConfig.json'),
	);

	// Post message
	{
		const channel = await connection.createChannel();

		await channel.assertQueue(clashq);
		const message = `processClash ${teamspace} ${project} $SHARED_SPACE/${correlationId}/clashConfig.json`;
		channel.sendToQueue(clashq, Buffer.from(message), {
			correlationId,
		});

		// Closing the channel flushes the message
		await channel.close();
	}

	// Wait for callbacks
	{
		const channel = await connection.createChannel();

		await channel.assertQueue(callbackq);
		await channel.prefetch(1);

		await new Promise((resolve, reject) => {
			let seenProcessing = false;

			const timeoutHandle = setTimeout(() => {
				reject(new Error('Timed out waiting for clash callback'));
			}, 60000);

			channel.consume(callbackq, (message) => {
				const content = JSON.parse(message.content.toString());

				if (message.properties.correlationId !== correlationId) {
					channel.ack(message);
					return;
				}

				if (content.status === PROCESSING && content.type === CLASH_TYPE) {
					assert.equal(content.teamspace, teamspace);
					assert.equal(content.project, project);
					seenProcessing = true;
					channel.ack(message);
					return;
				}

				assert.equal(message.properties.correlationId, correlationId);
				assert.equal(content.value, 0);
				assert.equal(content.results, path.join(`$SHARED_SPACE/${correlationId}`, 'results.json'));
				assert.equal(fs.existsSync(replaceSharedDirTag(content.results)), true);
				assert.equal(content.type, CLASH_TYPE);
				assert.equal(content.project, project);
				assert.equal(content.teamspace, teamspace);
				assert.equal(seenProcessing, true);

				clearTimeout(timeoutHandle);
				channel.ack(message);
				resolve();
			});
		});

		await channel.close();
	}

	await connection.close();
});
