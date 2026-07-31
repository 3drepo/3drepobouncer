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

const { describe, before, after, test } = require('node:test');
const assert = require('node:assert');
const ampq = require('amqplib');
const fs = require('fs');
const path = require('path');
const { CLASH, MODEL, DRAWING } = require('../../src/constants/queueLabels');
const { startBouncerWorker } = require('../helpers');
const { config, replaceSharedDirTag } = require('../../src/lib/config');
const { CLASH: CLASH_TYPE, IMPORT: IMPORT_TYPE, DRAWING: DRAWING_TYPE } = require('../../src/constants/messageTypes');
const queueLabels = require('../../src/constants/queueLabels');
const { generateUUIDString, postToQueue, waitForCallback, generateRandomString, createCorrelationIdAndSharedDirectory } = require('../helpers');

/*
* The test suite will start and stop its own bouncers, but expects the queue
* to already be running.
*/

const testClashQ = async () => {
	const connection = await ampq.connect(config.rabbitmq.host);

	// In practice, these are used to determine where to look for the clash run
	// in the database. They are completely independent of which geometries
	// are clashed, so can be anything here.

	const project = generateUUIDString();
	const teamspace = generateRandomString();

	// This config in the tests repo is a valid config that performs a clash
	// using one of the ClashDetection containers in the database dump. We
	// copy it to $SHARED_SPACE in order to test the shared space tag
	// substitution logic as well.

	const correlationId = createCorrelationIdAndSharedDirectory([
		path.join(process.env.REPO_MODEL_PATH, 'clash/simple_clash_config.json'),
	]);

	await postToQueue(
		connection,
		config.rabbitmq.clash_queue,
		`processClash ${teamspace} ${project} $SHARED_SPACE/${correlationId}/simple_clash_config.json`,
		correlationId,
	);

	await waitForCallback(connection, config.rabbitmq.callback_queue, correlationId, {
		timeoutMessage: 'Timed out waiting for clash callback',
		onProcessing: (content) => {
			assert.equal(content.type, CLASH_TYPE);
			assert.equal(content.teamspace, teamspace);
			assert.equal(content.project, project);
		},
		onComplete: (content, seenProcessing) => {
			assert.equal(content.value, 0);
			assert.equal(content.results, path.join('$SHARED_SPACE', `${correlationId}`, 'results.json'));
			assert.equal(fs.existsSync(replaceSharedDirTag(content.results)), true);
			assert.equal(content.type, CLASH_TYPE);
			assert.equal(content.project, project);
			assert.equal(content.teamspace, teamspace);
			assert.equal(seenProcessing, true);
		},
	});

	await connection.close();
};

const testModelQ = async () => {
	const connection = await ampq.connect(config.rabbitmq.host);
	const teamspace = generateRandomString();
	const container = generateUUIDString();

	// Create the import configuration, as the backend would. See,
	// src\v4\services\queue.js
	// src\v5\handler\queue.js
	// The import command should use the shared space placeholder literal, as
	// the backend does.

	const correlationId = createCorrelationIdAndSharedDirectory([
		path.join(process.env.REPO_MODEL_PATH, 'cube.obj'),
	]);

	const importConfig = {
		lod: 0,
		timezone: 'Europe/London',
		importAnimations: false,
		tag: 'model_import_test',
		owner: generateUUIDString(),
		units: 'm',
		file: `$SHARED_SPACE/${correlationId}/cube.obj`,
		teamspace,
		container,
		revId: generateUUIDString(),
	};

	fs.writeFileSync(
		path.join(config.rabbitmq.sharedDir, `${correlationId}.json`),
		JSON.stringify(importConfig),
	);

	await postToQueue(
		connection,
		config.rabbitmq.model_queue,
		`import -f $SHARED_SPACE/${correlationId}.json`,
		correlationId,
	);

	await waitForCallback(connection, config.rabbitmq.callback_queue, correlationId, {
		timeoutMessage: 'Timed out waiting for model import callback',
		onProcessing: (content) => {
			assert.equal(content.type, queueLabels.MODEL);
		},
		onComplete: (content, seenProcessing) => {
			assert.equal(content.value, 0);
			assert.equal(content.type, IMPORT_TYPE);
			assert.equal(content.teamspace, teamspace);
			assert.equal(content.container, container);
			assert.equal(content.user, importConfig.owner);
			assert.equal(seenProcessing, true);
		},
	});

	await connection.close();
};

const testDrawingQ = async () => {
	const connection = await ampq.connect(config.rabbitmq.host);

	const correlationId = generateUUIDString();

	// These arguments must match what is already in the database.

	const teamspace = 'testDrawings';
	const drawing = '0ffe7f18-8ef9-4580-9419-a1bcb9d4c9c7';

	// When bouncer processes drawings, it updates what should be an already
	// existing revision, with apprporiate refs for the source image.
	// The test database should have already been restored containing a
	// drawing revision with the following id.

	const drawingConfig = {
		owner: generateUUIDString(),
		drawing,
		teamspace,
		revId: '4dce0696-533b-490e-bd69-b437f24c7e3d',
	};

	fs.writeFileSync(
		path.join(config.rabbitmq.sharedDir, `${correlationId}.json`),
		JSON.stringify(drawingConfig),
	);

	await postToQueue(
		connection,
		config.rabbitmq.drawing_queue,
		`processDrawing $SHARED_SPACE/${correlationId}.json`,
		correlationId,
	);

	await waitForCallback(connection, config.rabbitmq.callback_queue, correlationId, {
		timeoutMessage: 'Timed out waiting for drawing callback',
		onProcessing: (content) => {
			assert.equal(content.teamspace, teamspace);
			assert.equal(content.drawing, drawing);
		},
		onComplete: (content, seenProcessing) => {
			assert.equal(content.value, 0);
			assert.equal(content.type, DRAWING_TYPE);
			assert.equal(content.teamspace, teamspace);
			assert.equal(content.drawing, drawing);
			assert.equal(content.user, drawingConfig.owner);
			assert.equal(seenProcessing, true);
		},
	});

	await connection.close();
};

const testSingleQConsumer = async (queue) => {
	const connection = await ampq.connect(config.rabbitmq.host);
	const channel = await connection.createChannel();
	const queueNames = {
		[queueLabels.MODEL]: config.rabbitmq.model_queue,
		[queueLabels.DRAWING]: config.rabbitmq.drawing_queue,
		[queueLabels.CLASH]: config.rabbitmq.clash_queue,
	};

	const expectedCounts = {};
	for (const queueName of Object.values(queueNames)) {
		expectedCounts[queueName] = 0;
	}
	expectedCounts[queueNames[queue]] = 1;

	const timeoutMS = 10000;
	const intervalMS = 250;
	const start = Date.now();

	let matched = false;
	while (Date.now() - start < timeoutMS) {
		const counts = {};
		for (const queueName of Object.keys(expectedCounts)) {
			// eslint-disable-next-line no-await-in-loop
			const { consumerCount } = await channel.checkQueue(queueName);
			counts[queueName] = consumerCount;
		}

		matched = Object.entries(expectedCounts)
			.every(([queueName, expected]) => counts[queueName] === expected);

		if (matched) {
			break;
		}

		// eslint-disable-next-line no-await-in-loop
		await new Promise((resolve) => setTimeout(resolve, intervalMS));
	}

	assert.equal(matched, true);

	await channel.close();
	await connection.close();
};

describe('Test all queues', () => {
	let stopBouncerWorker;

	before(async () => {
		const { stop } = await startBouncerWorker();
		stopBouncerWorker = stop;
	});

	after(async () => {
		await stopBouncerWorker();
	});

	test('clashQ', testClashQ);
	test('drawingQ', testDrawingQ);
	test('modelQ', testModelQ);
});

describe('Test single queues', () => {
	test('clashQ', async () => {
		const { stop } = await startBouncerWorker(CLASH);
		await testSingleQConsumer(CLASH);
		await stop();
	});

	test('modelQ', async () => {
		const { stop } = await startBouncerWorker(MODEL);
		await testSingleQConsumer(MODEL);
		await stop();
	});

	test('drawingQ', async () => {
		const { stop } = await startBouncerWorker(DRAWING);
		await testSingleQConsumer(DRAWING);
		await stop();
	});
});

describe('Test exitAfter', () => {
	test('clashQ', async () => {
		const { waitForExit } = await startBouncerWorker(CLASH, 1);
		await testClashQ();
		await waitForExit();
	});

	test('modelQ', async () => {
		const { waitForExit } = await startBouncerWorker(MODEL, 1);
		await testModelQ();
		await waitForExit();
	});

	test('drawingQ', async () => {
		const { waitForExit } = await startBouncerWorker(DRAWING, 1);
		await testDrawingQ();
		await waitForExit();
	});
});
