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
const { startBouncerWorker } = require('../helpers');
const { CLASH, MODEL, DRAWING } = require('../../src/constants/queueLabels');
const { testClashQ, testModelQ, testDrawingQ, testSingleQConsumer } = require('../tests');
/*
* The test suite will start and stop its own bouncers, but expects the queue
* to already be running.
*/

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
