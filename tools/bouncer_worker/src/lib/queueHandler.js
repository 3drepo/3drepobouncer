/**
 * Copyright (C) 2020 3D Repo Ltd
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

const amqp = require('amqplib');
const { rabbitmq } = require('./config').config;
const logger = require('./logger');
const { exitApplication, sleep } = require('./utils');
const JobQHandler = require('../queues/jobQueueHandler');
const ModelQHandler = require('../queues/modelQueueHandler');
const UnityQHandler = require('../queues/unityQueueHandler');

let connClosed = false;
let retry = 0;

const QueueHandler = {};

const listenToQueue = (channel, queueName, prefetchCount, callback) => {
	channel.assertQueue(queueName, { durable: true });
	logger.info('Bouncer Client Queue started. Waiting for messages in %s of %s....', queueName, rabbitmq.host);
	channel.prefetch(prefetchCount);
	channel.consume(queueName, async (msg) => {
		logger.info(' [x] Received %s from %s', msg.content.toString(), queueName);
		await callback(msg.content.toString(), msg.properties.correlationId,
			(reply, queue = rabbitmq.callback_queue) => {
				logger.info('sending to reply queue(%s): %s', queue, reply);
				// eslint-disable-next-line new-cap
				channel.sendToQueue(queue, new Buffer.from(reply),
					{ correlationId: msg.properties.correlationId, appId: msg.properties.appId });
			});
		channel.ack(msg);
	}, { noAck: false });
};

const establishChannel = async (conn, listenToJobQueue, listenToModelQueue, listenToUnityQueue) => {
	const channel = await conn.createChannel();
	channel.assertQueue(rabbitmq.callback_queue, { durable: true });
	if (listenToJobQueue && JobQHandler.validateConfiguration()) {
		listenToQueue(channel, rabbitmq.worker_queue, rabbitmq.task_prefetch, JobQHandler.onMessageReceived);
	}

	if (listenToModelQueue && ModelQHandler.validateConfiguration()) {
		listenToQueue(channel, rabbitmq.model_queue, rabbitmq.model_prefetch, ModelQHandler.onMessageReceived);
	}

	if (listenToUnityQueue && UnityQHandler.validateConfiguration()) {
		listenToQueue(channel, rabbitmq.unity_queue, rabbitmq.unity_prefetch, UnityQHandler.onMessageReceived);
	}
};

const executeOneTask = async (conn, queueName, callback) => {
	const channel = await conn.createChannel();
	channel.assertQueue(rabbitmq.callback_queue, { durable: true });
	channel.prefetch(1);
	const msg = await channel.get(queueName, { noAck: false });
	if (msg) {
		logger.info(' [x] Received %s from %s', msg.content.toString(), queueName);
		await callback(msg.content.toString(), msg.properties.correlationId,
			(reply, queue = rabbitmq.callback_queue) => {
				logger.info('sending to reply queue(%s): %s', queue, reply);
				// eslint-disable-next-line new-cap
				channel.sendToQueue(queue, new Buffer.from(reply),
					{ correlationId: msg.properties.correlationId, appId: msg.properties.appId });
			});
		channel.ack(msg);
		// There's no promise/callback for knowing when Ack has been received by the server. So we're doing a wait here.
		await sleep(rabbitmq.waitBeforeShutdownMS);
	} else {
		logger.info(`No message found in ${queueName}.`);
	}
	await channel.close();
	await conn.close();
	exitApplication(0);
};

const reconnect = (uponConnected) => {
	const maxRetries = rabbitmq.maxRetries || 3;
	if (++retry <= maxRetries) {
		logger.error(`[AMQP] Trying to reconnect [${retry}/${maxRetries}]...`);
		// eslint-disable-next-line no-use-before-define
		connectToRabbitMQ(true, uponConnected);
	} else {
		logger.error('[AMQP] Retries exhausted');
		exitApplication();
	}
};

const connectToRabbitMQ = async (autoReconnect, uponConnected) => {
	try {
		const conn = await amqp.connect(rabbitmq.host);
		retry = 0;
		connClosed = false;
		logger.info('[AMQP] Connected! Creating channel...');

		conn.on('close', () => {
			if (!connClosed) {
				// this can be called more than once for some reason. Use a boolean to distinguish first timers.
				connClosed = true;
				logger.info('[AMQP] connection closed.');
				if (autoReconnect) {
					reconnect(uponConnected);
				}
			}
		});

		conn.on('error', (err) => {
			logger.error(`[AMQP] connection error: ${err.message}`);
		});
		uponConnected(conn);
	} catch (err) {
		logger.error(`[AMQP] failed to establish connection to rabbit mq: ${err}.`);
		if (autoReconnect) {
			reconnect(uponConnected);
		}
	}
};

QueueHandler.connectToQueue = async (listenToJobQueue, listenToModelQueue, listenToUnityQueue) => {
	connectToRabbitMQ(true, (conn) => establishChannel(conn, listenToJobQueue, listenToModelQueue, listenToUnityQueue));
};

QueueHandler.runSingleTask = async (queueType) => {
	let queueName;
	let callback;
	switch (queueType) {
		case 'job':
			if (!JobQHandler.validateConfiguration()) {
				exitApplication();
			}
			queueName = rabbitmq.worker_queue;
			callback = JobQHandler.onMessageReceived;
			break;
		case 'model':
			if (!ModelQHandler.validateConfiguration()) {
				exitApplication();
			}
			queueName = rabbitmq.model_queue;
			callback = ModelQHandler.onMessageReceived;
			break;
		case 'unity':
			if (!UnityQHandler.validateConfiguration()) {
				exitApplication();
			}
			queueName = rabbitmq.unity_queue;
			callback = UnityQHandler.onMessageReceived;
			break;
		default:
			logger.error(`unrecognised queue type: ${queueType}. Expected [job|model|unity]`);
			exitApplication();
	}
	connectToRabbitMQ(false, (conn) => executeOneTask(conn, queueName, callback));
};

module.exports = QueueHandler;
