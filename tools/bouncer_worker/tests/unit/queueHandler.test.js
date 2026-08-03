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

const { ERRCODE_REPO_LICENCE_INVALID } = require('../../src/constants/errorCodes');

const moduleRequire = createRequire(__filename);

const queueHandlerPath = require.resolve('../../src/lib/queueHandler');
const amqpPath = require.resolve('amqplib');
const fsPath = require.resolve('fs');
const configPath = require.resolve('../../src/lib/config');
const loggerPath = require.resolve('../../src/lib/logger');
const utilsPath = require.resolve('../../src/lib/utils');
const messageDecoderPath = require.resolve('../../src/lib/messageDecoder');
const bouncerClientPath = require.resolve('../../src/tasks/bouncerClient');
const processMonitorPath = require.resolve('../../src/lib/processMonitor');
const modelHandlerPath = require.resolve('../../src/queues/modelQueueHandler');
const drawingHandlerPath = require.resolve('../../src/queues/drawingQueueHandler');
const clashHandlerPath = require.resolve('../../src/queues/clashQueueHandler');

const clearModuleCache = () => {
	delete require.cache[queueHandlerPath];
	delete require.cache[amqpPath];
	delete require.cache[fsPath];
	delete require.cache[configPath];
	delete require.cache[loggerPath];
	delete require.cache[utilsPath];
	delete require.cache[messageDecoderPath];
	delete require.cache[bouncerClientPath];
	delete require.cache[processMonitorPath];
	delete require.cache[modelHandlerPath];
	delete require.cache[drawingHandlerPath];
	delete require.cache[clashHandlerPath];
};

afterEach(clearModuleCache);

const flushAsync = () => new Promise((resolve) => setImmediate(resolve));

const loadQueueHandlerWithMocks = ({
	connectImpl,
	modelOnMessage,
	drawingOnMessage,
	clashOnMessage,
	rabbitOverrides,
	exitImpl,
	useRealHandlers = false,
	messageDecoderImpl,
	runBouncerImpl,
} = {}) => {
	const calls = {
		connect: [],
		assertQueue: [],
		prefetch: [],
		consumeQueues: [],
		sendToQueue: [],
		ack: [],
		nack: [],
		get: [],
		waitForConfirms: 0,
		channelClose: 0,
		connClose: 0,
		sleep: [],
		exitApplication: [],
		messageDecoder: [],
		runBouncer: [],
		sendReport: [],
		clearReport: [],
		statSync: [],
		gatherProcessInformation: [],
	};

	const logs = {
		info: [],
		error: [],
		verbose: [],
	};

	const consumers = {};
	const eventHandlers = {};

	const channel = {
		assertQueue: (queueName) => calls.assertQueue.push(queueName),
		prefetch: (count) => calls.prefetch.push(count),
		consume: (queueName, callback) => {
			calls.consumeQueues.push(queueName);
			consumers[queueName] = callback;
		},
		sendToQueue: (queueName, buffer, props) => calls.sendToQueue.push({ queueName, buffer, props }),
		ack: (msg) => calls.ack.push(msg),
		nack: (msg) => calls.nack.push(msg),
		get: async (queueName) => {
			calls.get.push(queueName);
			return null;
		},
		waitForConfirms: async () => {
			calls.waitForConfirms += 1;
		},
		close: async () => {
			calls.channelClose += 1;
		},
	};

	const conn = {
		on: (event, cb) => {
			eventHandlers[event] = cb;
		},
		createChannel: async () => channel,
		createConfirmChannel: async () => channel,
		close: async () => {
			calls.connClose += 1;
		},
	};

	require.cache[amqpPath] = {
		id: amqpPath,
		filename: amqpPath,
		loaded: true,
		exports: {
			connect: async (host) => {
				calls.connect.push(host);
				if (connectImpl) {
					return connectImpl({ conn, channel, calls, consumers, eventHandlers });
				}
				return conn;
			},
		},
	};

	require.cache[configPath] = {
		id: configPath,
		filename: configPath,
		loaded: true,
		exports: {
			replaceSharedDirTag: (value) => value,
			config: {
				logging: {
					taskLogDir: '/tmp/task-logs',
				},
				repoLicense: 'test-license',
				rabbitmq: {
					host: 'amqp://test-rabbit',
					callback_queue: 'callbackq',
					model_queue: 'modelq',
					drawing_queue: 'drawingq',
					clash_queue: 'clashq',
					model_prefetch: 2,
					drawing_prefetch: 3,
					task_prefetch: 4,
					maxRetries: 2,
					maxWaitTimeMS: 0,
					pollingIntervalMS: 1,
					...rabbitOverrides,
				},
			},
		},
	};

	require.cache[fsPath] = {
		id: fsPath,
		filename: fsPath,
		loaded: true,
		exports: {
			statSync: (filepath) => {
				calls.statSync.push(filepath);
				return { size: 321 };
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

	require.cache[utilsPath] = {
		id: utilsPath,
		filename: utilsPath,
		loaded: true,
		exports: {
			exitApplication: (code) => {
				calls.exitApplication.push(code);
				if (exitImpl) {
					return exitImpl(code);
				}
				return undefined;
			},
			sleep: async (ms) => calls.sleep.push(ms),
			gatherProcessInformation: (...args) => {
				calls.gatherProcessInformation.push(args);
				return { Rid: args[5] };
			},
		},
	};

	require.cache[messageDecoderPath] = {
		id: messageDecoderPath,
		filename: messageDecoderPath,
		loaded: true,
		exports: {
			messageDecoder: (cmd) => {
				calls.messageDecoder.push(cmd);
				if (messageDecoderImpl) {
					return messageDecoderImpl(cmd);
				}

				if (cmd.startsWith('import')) {
					return {
						teamspace: 'ts-model',
						container: 'container-model',
						user: 'user-model',
						cmdParams: ['import', '-f', '/tmp/model.ifc'],
						file: '/tmp/model.ifc',
					};
				}

				if (cmd.startsWith('processDrawing')) {
					return {
						teamspace: 'ts-drawing',
						drawing: 'drawing-1',
						user: 'user-drawing',
						cmdParams: ['processDrawing', '/tmp/drawing.json'],
						format: 'png',
						size: 100,
					};
				}

				if (cmd.startsWith('processClash')) {
					return {
						teamspace: 'ts-clash',
						project: 'project-1',
						cmdParams: ['--cfg'],
						configFile: '/tmp/config.json',
					};
				}

				return { errorCode: 16 };
			},
		},
	};

	require.cache[bouncerClientPath] = {
		id: bouncerClientPath,
		filename: bouncerClientPath,
		loaded: true,
		exports: {
			runBouncerCommand: async (...args) => {
				calls.runBouncer.push(args);
				if (runBouncerImpl) {
					return runBouncerImpl(...args);
				}
				return 0;
			},
		},
	};

	require.cache[processMonitorPath] = {
		id: processMonitorPath,
		filename: processMonitorPath,
		loaded: true,
		exports: {
			sendReport: async (rid) => calls.sendReport.push(rid),
			clearReport: async (rid) => calls.clearReport.push(rid),
		},
	};

	if (!useRealHandlers) {
		require.cache[modelHandlerPath] = {
			id: modelHandlerPath,
			filename: modelHandlerPath,
			loaded: true,
			exports: {
				prefetchCount: 2,
				onMessageReceived: modelOnMessage || (async (_cmd, _rid, reply) => reply('{"ok":true}')),
			},
		};

		require.cache[drawingHandlerPath] = {
			id: drawingHandlerPath,
			filename: drawingHandlerPath,
			loaded: true,
			exports: {
				prefetchCount: 3,
				onMessageReceived: drawingOnMessage || (async () => {}),
			},
		};

		require.cache[clashHandlerPath] = {
			id: clashHandlerPath,
			filename: clashHandlerPath,
			loaded: true,
			exports: {
				prefetchCount: 4,
				onMessageReceived: clashOnMessage || (async () => {}),
			},
		};
	}

	const QueueHandler = moduleRequire(queueHandlerPath);
	return { QueueHandler, calls, logs, consumers, conn, channel, eventHandlers };
};

describe('queueHandler.js', () => {
	test('connectToQueue wires all configured queues and handles successful message ack', async () => {
		const { QueueHandler, calls, consumers } = loadQueueHandlerWithMocks();

		await QueueHandler.connectToQueue();
		await flushAsync();

		assert.deepEqual(calls.connect, ['amqp://test-rabbit']);
		assert.equal(calls.assertQueue.includes('callbackq'), true);
		assert.deepEqual(calls.consumeQueues.sort(), ['clashq', 'drawingq', 'modelq']);
		assert.deepEqual(calls.prefetch, [2, 3, 4]);

		const message = {
			content: Buffer.from('import -f foo.json'),
			properties: { correlationId: 'rid-1', appId: 'app-1' },
		};

		await consumers.modelq(message);
		await flushAsync();

		assert.equal(calls.sendToQueue.length, 1);
		assert.equal(calls.sendToQueue[0].queueName, 'callbackq');
		assert.equal(calls.sendToQueue[0].props.correlationId, 'rid-1');
		assert.equal(calls.ack.length, 1);
		assert.equal(calls.nack.length, 0);
	});

	test('nacks message when handler rejects during consume callback', async () => {
		const { QueueHandler, calls, consumers } = loadQueueHandlerWithMocks({
			modelOnMessage: async () => Promise.reject(new Error('handler failed')),
		});

		await QueueHandler.connectToQueue('model');
		await flushAsync();

		const message = {
			content: Buffer.from('import -f foo.json'),
			properties: { correlationId: 'rid-2', appId: 'app-2' },
		};

		await consumers.modelq(message);
		await flushAsync();

		assert.equal(calls.ack.length, 0);
		assert.equal(calls.nack.length, 1);
	});

	test('runNTasks processes queued message and closes resources', async () => {
		const { QueueHandler, calls, channel } = loadQueueHandlerWithMocks();
		const msg = {
			content: Buffer.from('import -f bar.json'),
			properties: { correlationId: 'rid-3', appId: 'app-3' },
		};
		const pending = [msg];
		channel.get = async () => pending.shift() || null;

		await QueueHandler.runNTasks('model', 1);
		await flushAsync();

		assert.equal(calls.waitForConfirms, 1);
		assert.equal(calls.channelClose, 1);
		assert.equal(calls.connClose, 1);
		assert.equal(calls.ack.length, 1);
	});

	test('reconnects after initial connection failure when autoReconnect is enabled', async () => {
		let attempt = 0;
		const { QueueHandler, calls, logs } = loadQueueHandlerWithMocks({
			connectImpl: async ({ conn }) => {
				attempt += 1;
				if (attempt === 1) {
					throw new Error('first connect failed');
				}
				return conn;
			},
		});

		await QueueHandler.connectToQueue('drawing');
		await flushAsync();
		await flushAsync();

		assert.deepEqual(calls.connect, ['amqp://test-rabbit', 'amqp://test-rabbit']);
		assert.equal(logs.error.some(([msg]) => msg.includes('Trying to reconnect[1/2]...')), true);
	});

	test('exits when queue label is invalid', async () => {
		const sentinel = new Error('exit');
		const { QueueHandler, calls, logs } = loadQueueHandlerWithMocks({
			exitImpl: () => {
				throw sentinel;
			},
		});

		await assert.rejects(() => QueueHandler.connectToQueue('not-a-queue'), sentinel);
		assert.equal(calls.connect.length, 0);
		assert.equal(logs.error.some(([msg]) => msg.includes('Unrecognised queue type: not-a-queue')), true);
	});

	test('exits after retries exhausted when reconnect keeps failing', async () => {
		const { QueueHandler, logs, calls } = loadQueueHandlerWithMocks({
			connectImpl: async () => {
				throw new Error('connect failed always');
			},
			exitImpl: () => undefined,
		});

		await QueueHandler.connectToQueue('clash');
		await flushAsync();
		await flushAsync();
		assert.equal(logs.error.some(([msg]) => msg.includes('Retries exhausted')), true);
		assert.equal(calls.exitApplication.length > 0, true);
	});

	test('exits when model queue is missing from config mapping', async () => {
		const sentinel = new Error('exit');
		const { QueueHandler, logs } = loadQueueHandlerWithMocks({
			rabbitOverrides: { model_queue: undefined },
			exitImpl: () => {
				throw sentinel;
			},
		});

		await assert.rejects(() => QueueHandler.connectToQueue('model'), sentinel);
		assert.equal(logs.error.some(([msg]) => msg.includes('Failed to find rabbitmq entry for queue type: model in config')), true);
	});

	test('exits when drawing queue is missing from config mapping', async () => {
		const sentinel = new Error('exit');
		const { QueueHandler, logs } = loadQueueHandlerWithMocks({
			rabbitOverrides: { drawing_queue: undefined },
			exitImpl: () => {
				throw sentinel;
			},
		});

		await assert.rejects(() => QueueHandler.connectToQueue('drawing'), sentinel);
		assert.equal(logs.error.some(([msg]) => msg.includes('Failed to find rabbitmq entry for queue type: drawing in config')), true);
	});

	test('exits when clash queue is missing from config mapping', async () => {
		const sentinel = new Error('exit');
		const { QueueHandler, logs } = loadQueueHandlerWithMocks({
			rabbitOverrides: { clash_queue: undefined },
			exitImpl: () => {
				throw sentinel;
			},
		});

		await assert.rejects(() => QueueHandler.connectToQueue('clash'), sentinel);
		assert.equal(logs.error.some(([msg]) => msg.includes('Failed to find rabbitmq entry for queue type: clash in config')), true);
	});

	test('runNTasks retries on empty queue then processes next message', async () => {
		const { QueueHandler, calls, channel, logs } = loadQueueHandlerWithMocks({
			rabbitOverrides: { maxWaitTimeMS: 5000, pollingIntervalMS: 25 },
		});
		const msg = {
			content: Buffer.from('import -f delayed.json'),
			properties: { correlationId: 'rid-4', appId: 'app-4' },
		};
		let getCall = 0;
		channel.get = async () => {
			getCall += 1;
			return getCall === 1 ? null : msg;
		};

		await QueueHandler.runNTasks('model', 1);
		await flushAsync();

		assert.deepEqual(calls.sleep, [25]);
		assert.equal(calls.ack.length, 1);
		assert.equal(logs.verbose.some(([msgText]) => msgText.includes('No message found in modelq. Retrying in 0.025s')), true);
	});

	test('runNTasks stops when max wait time is reached with no message', async () => {
		const { QueueHandler, calls, channel, logs } = loadQueueHandlerWithMocks({
			rabbitOverrides: { maxWaitTimeMS: -1 },
		});
		channel.get = async () => null;

		await QueueHandler.runNTasks('drawing', 1);
		await flushAsync();

		assert.equal(calls.ack.length, 0);
		assert.equal(calls.nack.length, 0);
		assert.equal(logs.info.some(([msg]) => msg.includes('No message found in drawingq. Max wait time reached.')), true);
	});

	test('runNTasks nacks when callback rejects', async () => {
		const { QueueHandler, calls, channel } = loadQueueHandlerWithMocks({
			modelOnMessage: async () => Promise.reject(new Error('task failure')),
		});
		const msg = {
			content: Buffer.from('import -f failed.json'),
			properties: { correlationId: 'rid-5', appId: 'app-5' },
		};
		channel.get = async () => msg;

		await QueueHandler.runNTasks('model', 1);
		await flushAsync();

		assert.equal(calls.ack.length, 0);
		assert.equal(calls.nack.length, 1);
	});

	test('handles connection close event with auto reconnect path', async () => {
		const { QueueHandler, eventHandlers, logs, calls } = loadQueueHandlerWithMocks();

		await QueueHandler.connectToQueue('model');
		await flushAsync();

		eventHandlers.close();
		await flushAsync();

		assert.equal(logs.info.some(([msg]) => msg.includes('Connection closed.')), true);
		assert.equal(calls.connect.length, 2);
	});

	test('handles connection close and error events when autoReconnect is false', async () => {
		const { QueueHandler, eventHandlers, calls, logs, channel } = loadQueueHandlerWithMocks();
		channel.get = async () => null;

		await QueueHandler.runNTasks('clash', 1);
		await flushAsync();

		eventHandlers.error({ message: 'socket broken' });
		eventHandlers.close();

		assert.equal(logs.error.some(([msg]) => msg.includes('Connection error: socket broken')), true);
		assert.equal(calls.exitApplication.includes(0), true);
	});

	test('exits without reconnect when runNTasks fails to establish connection', async () => {
		const { QueueHandler, calls, logs } = loadQueueHandlerWithMocks({
			connectImpl: async () => {
				throw new Error('cannot connect');
			},
			exitImpl: () => undefined,
		});

		await QueueHandler.runNTasks('model', 1);
		await flushAsync();
		assert.equal(calls.connect.length, 1);
		assert.equal(calls.exitApplication.length > 0, true);
		assert.equal(logs.error.some(([msg]) => msg.includes('Failed to establish connection to rabbit mq: Error: cannot connect.')), true);
	});

	test('routes model queue messages through real model handler', async () => {
		const { QueueHandler, consumers, calls } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			runBouncerImpl: async () => 7,
		});

		await QueueHandler.connectToQueue('model');
		await flushAsync();

		const message = {
			content: Buffer.from('import -f model.json'),
			properties: { correlationId: 'rid-model', appId: 'app-model' },
		};

		await consumers.modelq(message);
		await flushAsync();

		assert.equal(calls.runBouncer.length, 1);
		assert.equal(calls.sendReport.includes('rid-model'), true);
		assert.equal(calls.sendToQueue.length, 2);
		assert.equal(calls.statSync.includes('/tmp/model.ifc'), true);

		const processingReply = JSON.parse(calls.sendToQueue[0].buffer.toString());
		const finalReply = JSON.parse(calls.sendToQueue[1].buffer.toString());
		assert.equal(processingReply.status, 'processing');
		assert.equal(finalReply.value, 7);
		assert.equal(finalReply.type, 'import');
		assert.equal(calls.ack.length, 1);
	});

	test('routes drawing queue messages through real drawing handler', async () => {
		const { QueueHandler, consumers, calls } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			runBouncerImpl: async () => 5,
		});

		await QueueHandler.connectToQueue('drawing');
		await flushAsync();

		const message = {
			content: Buffer.from('processDrawing /tmp/drawing.json'),
			properties: { correlationId: 'rid-drawing', appId: 'app-drawing' },
		};

		await consumers.drawingq(message);
		await flushAsync();

		assert.equal(calls.runBouncer.length, 1);
		assert.equal(calls.sendReport.includes('rid-drawing'), true);
		assert.equal(calls.sendToQueue.length, 2);

		const processingReply = JSON.parse(calls.sendToQueue[0].buffer.toString());
		const finalReply = JSON.parse(calls.sendToQueue[1].buffer.toString());
		assert.equal(processingReply.status, 'processing');
		assert.equal(finalReply.value, 5);
		assert.equal(finalReply.type, 'drawing');
		assert.equal(calls.ack.length, 1);
	});

	test('routes clash queue messages through real clash handler', async () => {
		const { QueueHandler, consumers, calls } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			runBouncerImpl: async () => 3,
		});

		await QueueHandler.connectToQueue('clash');
		await flushAsync();

		const message = {
			content: Buffer.from('processClash ts-clash proj-clash /tmp/config.json'),
			properties: { correlationId: 'rid-clash', appId: 'app-clash' },
		};

		await consumers.clashq(message);
		await flushAsync();

		assert.equal(calls.runBouncer.length, 1);
		assert.equal(calls.sendReport.includes('rid-clash'), true);
		assert.equal(calls.sendToQueue.length, 2);

		const processingReply = JSON.parse(calls.sendToQueue[0].buffer.toString());
		const finalReply = JSON.parse(calls.sendToQueue[1].buffer.toString());
		assert.equal(processingReply.status, 'processing');
		assert.equal(finalReply.value, 3);
		assert.equal(finalReply.type, 'clash');
		assert.equal(calls.ack.length, 1);
	});

	test('real model handler returns early on decode error', async () => {
		const { QueueHandler, consumers, calls } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			messageDecoderImpl: () => ({ errorCode: 16 }),
		});

		await QueueHandler.connectToQueue('model');
		await flushAsync();

		await consumers.modelq({
			content: Buffer.from('bad message'),
			properties: { correlationId: 'rid-model-err', appId: 'app-model' },
		});
		await flushAsync();

		assert.equal(calls.sendToQueue.length, 1);
		assert.deepEqual(JSON.parse(calls.sendToQueue[0].buffer.toString()), { value: 16 });
		assert.equal(calls.ack.length, 1);
	});

	test('real model handler reports default bouncer errors', async () => {
		const { QueueHandler, consumers, calls, logs } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			runBouncerImpl: async () => {
				throw new Error('model failed');
			},
		});

		await QueueHandler.connectToQueue('model');
		await flushAsync();

		await consumers.modelq({
			content: Buffer.from('import -f model.json'),
			properties: { correlationId: 'rid-model-fail', appId: 'app-model' },
		});
		await flushAsync();

		assert.equal(calls.sendReport.includes('rid-model-fail'), true);
		assert.equal(logs.error.some(([msg]) => msg.includes('Import model error: model failed')), true);
		assert.equal(calls.sendToQueue.length, 2);
		assert.equal(calls.ack.length, 1);
	});

	test('real model handler handles licence invalid as nack path', async () => {
		const { QueueHandler, consumers, calls, logs } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			rabbitOverrides: { maxWaitTimeMS: 50 },
			runBouncerImpl: async () => {
				throw ERRCODE_REPO_LICENCE_INVALID;
			},
		});

		await QueueHandler.connectToQueue('model');
		await flushAsync();

		await consumers.modelq({
			content: Buffer.from('import -f model.json'),
			properties: { correlationId: 'rid-model-lic', appId: 'app-model' },
		});
		await flushAsync();

		assert.equal(calls.clearReport.includes('rid-model-lic'), true);
		assert.equal(logs.error.some(([msg]) => msg.includes('Invalid 3D Repo license')), true);
		assert.equal(calls.nack.length, 1);
		assert.equal(calls.sleep.includes(50), true);
	});

	test('real drawing handler returns early on decode error', async () => {
		const { QueueHandler, consumers, calls } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			messageDecoderImpl: () => ({ errorCode: 16 }),
		});

		await QueueHandler.connectToQueue('drawing');
		await flushAsync();

		await consumers.drawingq({
			content: Buffer.from('bad drawing message'),
			properties: { correlationId: 'rid-drawing-err', appId: 'app-drawing' },
		});
		await flushAsync();

		assert.equal(calls.sendToQueue.length, 1);
		assert.deepEqual(JSON.parse(calls.sendToQueue[0].buffer.toString()), { value: 16 });
		assert.equal(calls.ack.length, 1);
	});

	test('real drawing handler handles licence invalid as nack path', async () => {
		const { QueueHandler, consumers, calls, logs } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			rabbitOverrides: { maxWaitTimeMS: 50 },
			runBouncerImpl: async () => {
				throw ERRCODE_REPO_LICENCE_INVALID;
			},
		});

		await QueueHandler.connectToQueue('drawing');
		await flushAsync();

		await consumers.drawingq({
			content: Buffer.from('processDrawing /tmp/drawing.json'),
			properties: { correlationId: 'rid-drawing-lic', appId: 'app-drawing' },
		});
		await flushAsync();

		assert.equal(calls.clearReport.includes('rid-drawing-lic'), true);
		assert.equal(logs.error.some(([msg]) => msg.includes('Invalid 3D Repo license')), true);
		assert.equal(calls.nack.length, 1);
		assert.equal(calls.sleep.includes(50), true);
	});

	test('real drawing handler reports default bouncer errors', async () => {
		const { QueueHandler, consumers, calls, logs } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			runBouncerImpl: async () => {
				throw new Error('drawing failed');
			},
		});

		await QueueHandler.connectToQueue('drawing');
		await flushAsync();

		await consumers.drawingq({
			content: Buffer.from('processDrawing /tmp/drawing.json'),
			properties: { correlationId: 'rid-drawing-fail', appId: 'app-drawing' },
		});
		await flushAsync();

		assert.equal(calls.sendReport.includes('rid-drawing-fail'), true);
		assert.equal(logs.error.some(([msg]) => msg.includes('Import drawing error: drawing failed')), true);
		assert.equal(calls.sendToQueue.length, 2);
		assert.equal(calls.ack.length, 1);
	});

	test('real clash handler returns early on decode error', async () => {
		const { QueueHandler, consumers, calls } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			messageDecoderImpl: () => ({ errorCode: 16 }),
		});

		await QueueHandler.connectToQueue('clash');
		await flushAsync();

		await consumers.clashq({
			content: Buffer.from('bad clash message'),
			properties: { correlationId: 'rid-clash-err', appId: 'app-clash' },
		});
		await flushAsync();

		assert.equal(calls.sendToQueue.length, 1);
		assert.deepEqual(JSON.parse(calls.sendToQueue[0].buffer.toString()), { value: 16 });
		assert.equal(calls.ack.length, 1);
	});

	test('real clash handler reports default bouncer errors', async () => {
		const { QueueHandler, consumers, calls, logs } = loadQueueHandlerWithMocks({
			useRealHandlers: true,
			runBouncerImpl: async () => {
				throw new Error('clash failed');
			},
		});

		await QueueHandler.connectToQueue('clash');
		await flushAsync();

		await consumers.clashq({
			content: Buffer.from('processClash ts-clash proj-clash /tmp/config.json'),
			properties: { correlationId: 'rid-clash-fail', appId: 'app-clash' },
		});
		await flushAsync();

		assert.equal(logs.error.some(([msg]) => msg.includes('Error running clash detection: clash failed')), true);
		assert.equal(calls.sendToQueue.length, 2);
		const finalReply = JSON.parse(calls.sendToQueue[1].buffer.toString());
		assert.equal(finalReply.message, 'clash failed');
		assert.equal(calls.ack.length, 1);
	});
});
