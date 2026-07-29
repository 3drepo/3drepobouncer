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
const { config } = require('../src/lib/config');
const { startBouncerWorker } = require('./helpers');
const { DRAWING } = require('../src/constants/queueLabels');
const { PROCESSING } = require('../src/constants/statuses');
const { DRAWING: DRAWING_TYPE } = require('../src/constants/messageTypes');

let stopBouncerWorker = null;

before(async () => {
	const { stop } = await startBouncerWorker(config, DRAWING);
	stopBouncerWorker = stop;
});

after(async () => {
	await stopBouncerWorker();
});

test('Test Drawing Q', { concurrency: false }, async () => {
	const drawingq = config.rabbitmq.drawing_queue;
	const callbackq = config.rabbitmq.callback_queue;

	const connection = await ampq.connect(config.rabbitmq.host);

	const correlationId = crypto.randomUUID().toString();

	const teamspace = 'testDrawings';
	const drawing = '0ffe7f18-8ef9-4580-9419-a1bcb9d4c9c7';

	// When bouncer processes drawings, it updates what should be an already
	// existing revision, with apprporiate refs for the source image.
	// The test database should have already been restored containing a
	// drawing revision with the following id.

	const drawingConfig = {
		owner: crypto.randomUUID().toString(),
		drawing,
		teamspace,
		revId: '4dce0696-533b-490e-bd69-b437f24c7e3d',
	};

	fs.writeFileSync(
		path.join(config.rabbitmq.sharedDir, `${correlationId}.json`),
		JSON.stringify(drawingConfig),
	);

	// Post message
	{
		const channel = await connection.createChannel();

		await channel.assertQueue(drawingq);
		const message = `processDrawing $SHARED_SPACE/${correlationId}.json`;
		channel.sendToQueue(drawingq, Buffer.from(message), {
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
				reject(new Error('Timed out waiting for drawing callback'));
			}, 60000);

			channel.consume(callbackq, (message) => {
				const content = JSON.parse(message.content.toString());

				if (message.properties.correlationId !== correlationId) {
					channel.ack(message);
					return;
				}

				if (content.status === PROCESSING) {
					assert.equal(content.teamspace, teamspace);
					assert.equal(content.drawing, drawing);
					seenProcessing = true;
					channel.ack(message);
					return;
				}

				assert.equal(content.value, 0);
				assert.equal(content.type, DRAWING_TYPE);
				assert.equal(content.teamspace, teamspace);
				assert.equal(content.drawing, drawing);
				assert.equal(content.user, drawingConfig.owner);
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
