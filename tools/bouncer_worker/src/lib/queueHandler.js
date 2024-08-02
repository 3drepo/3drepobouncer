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
const DrawingQHandler = require('../queues/drawingQueueHandler');

let connClosed = false;
let retry = 0;

const logLabel = { label: 'AMQP' };

const QueueHandler = {};

const queueLabel = {
	JOB: 'job',
	MODEL: 'model',
	DRAWING: 'drawing',
};

const queueHandlers = {};
if(rabbitmq.worker_queue) {
	queueHandlers[rabbitmq.worker_queue] = JobQHandler;
}
if(rabbitmq.model_queue) {
	queueHandlers[rabbitmq.model_queue] = ModelQHandler;
}
if(rabbitmq.drawing_queue) {
	queueHandlers[rabbitmq.drawing_queue] = DrawingQHandler;
}

const getQueueName = (label) => {
	switch (label) {
		case queueLabel.JOB:
			if(rabbitmq.worker_queue) {
				return rabbitmq.worker_queue;
			}
			break;
		case queueLabel.MODEL:
			if(rabbitmq.model_queue) {
				return rabbitmq.model_queue;
			}
			break;
		case queueLabel.DRAWING:
			if(rabbitmq.drawing_queue) {
				return rabbitmq.drawing_queue;
			}
			break;
		default:
			logger.error(`Unrecognised queue type: ${label}. Expected [job|model|drawing]`, logLabel);
			exitApplication();
	}
	logger.error(`Failed to find rabbitmq entry for queue type: ${label} in config`, logLabel);
	exitApplication();
}

const listenToQueue = (channel, queueName, prefetchCount, callback) => {
	channel.assertQueue(queueName, { durable: true });
	logger.info(`Waiting for messages in ${queueName}...`, logLabel);
	channel.prefetch(prefetchCount);
	channel.consume(queueName, async (msg) => {
		logger.info(`Received ${msg.content.toString()} from ${queueName}`, logLabel);
		callback(msg.content.toString(), msg.properties.correlationId, (reply, queue = rabbitmq.callback_queue) => {
			logger.info(`Sending reply to ${queue}: ${reply}`, logLabel);
			// eslint-disable-next-line new-cap
			channel.sendToQueue(queue, new Buffer.from(reply),
				{ correlationId: msg.properties.correlationId, appId: msg.properties.appId });
		}).then(() => {
			channel.ack(msg);
		}).catch((err) => {
			logger.error(`Message ${msg.properties.correlationId} was placed back into the queue due to error ${err}`, logLabel);
			channel.nack(msg);
		});
	}, { noAck: false });
};

const establishChannel = async (conn, queueNames) => {
	const channel = await conn.createChannel();
	channel.assertQueue(rabbitmq.callback_queue, { durable: true });
	queueNames.forEach(queueName => {
		const handler = queueHandlers[queueName];
		if(!handler.validateConfiguration(logLabel)) {
			exitApplication();
		}
		listenToQueue(channel, queueName, handler.prefetchCount, handler.onMessageReceived);
	})
};

const executeTasks = async (conn, queueName, nTasks, callback) => {
	const channel = await conn.createChannel();
	channel.assertQueue(rabbitmq.callback_queue, { durable: true });
	channel.prefetch(1);
	logger.info(`Processing ${nTasks} tasks on ${queueName}...`, logLabel);

	let lastActive = new Date();
	let currentTaskNum = 1;
	while (currentTaskNum <= nTasks) {
		// eslint-disable-next-line no-await-in-loop
		const msg = await channel.get(queueName, { noAck: false });
		if (msg) {
			logger.info(`[${currentTaskNum}/${nTasks}] Received ${msg.content.toString()} from ${queueName}`, logLabel);
			// eslint-disable-next-line no-await-in-loop
			await callback(
				msg.content.toString(),
				msg.properties.correlationId,
				// eslint-disable-next-line no-loop-func
				(reply, queue = rabbitmq.callback_queue) => {
					logger.info(`[${currentTaskNum}/${nTasks}] Sending to reply to ${queue}: ${reply}`, logLabel);
					// eslint-disable-next-line new-cap
					channel.sendToQueue(queue, new Buffer.from(reply),
						{ correlationId: msg.properties.correlationId, appId: msg.properties.appId });
				},
			).then(() => {
				channel.ack(msg);
			}).catch((err) => {
				logger.error(`Message ${msg.properties.correlationId} was placed back into the queue due to error ${err}`, logLabel);
				channel.nack(msg);
			});
			lastActive = new Date();
			++currentTaskNum;
		} else if (new Date() - lastActive > rabbitmq.maxWaitTimeMS) {
			logger.info(`[${currentTaskNum}/${nTasks}] No message found in ${queueName}. Max wait time reached.`, logLabel);
			break;
		} else {
			const waitTime = rabbitmq.pollingIntervalMS;
			logger.verbose(`[${currentTaskNum}/${nTasks}] No message found in ${queueName}. Retrying in ${waitTime / 1000}s`, logLabel);

			// eslint-disable-next-line no-await-in-loop
			await sleep(waitTime);
		}
	}

	// There's no promise/callback for knowing when Ack has been received by the server. So we're doing a wait here.
	await sleep(rabbitmq.waitBeforeShutdownMS);
	await channel.close();
	await conn.close();
};

const reconnect = (uponConnected) => {
	if (++retry <= rabbitmq.maxRetries) {
		logger.error(`Trying to reconnect[${retry}/${rabbitmq.maxRetries}]...`, logLabel);
		// eslint-disable-next-line no-use-before-define
		connectToRabbitMQ(true, uponConnected);
	} else {
		logger.error('Retries exhausted', logLabel);
		exitApplication();
	}
};

const connectToRabbitMQ = async (autoReconnect, uponConnected) => {
	try {
		logger.info(`Connecting to ${rabbitmq.host}...`, logLabel);
		const conn = await amqp.connect(rabbitmq.host);
		retry = 0;
		connClosed = false;

		conn.on('close', () => {
			if (!connClosed) {
				// this can be called more than once for some reason. Use a boolean to distinguish first timers.
				connClosed = true;
				logger.info('Connection closed.', logLabel);
				if (autoReconnect) {
					reconnect(uponConnected);
				} else {
					exitApplication(0);
				}
			}
		});

		conn.on('error', (err) => {
			logger.error(`Connection error: ${err.message}`, logLabel);
		});
		uponConnected(conn);
	} catch (err) {
		logger.error(`Failed to establish connection to rabbit mq: ${err}.`, logLabel);
		if (autoReconnect) {
			reconnect(uponConnected);
		} else {
			exitApplication();
		}
	}
};

QueueHandler.connectToQueue = async (specificQueue) => {
	const queueNames = [];
	if (specificQueue) {
		queueNames.push(getQueueName(specificQueue));
	} else {
		for (const label of Object.values(queueLabel)) {
			queueNames.push(getQueueName(label));
		}
	}
	connectToRabbitMQ(true,
		(conn) => establishChannel(conn, queueNames));
};

QueueHandler.runNTasks = async (queueType, nTasks) => {
	const queueName = getQueueName(queueType);
	const handler = queueHandlers[queueName];
	if(!handler.validateConfiguration(logLabel)) {
		exitApplication();
	}
	connectToRabbitMQ(false, (conn) => executeTasks(conn, queueName, nTasks, handler.onMessageReceived));
};

module.exports = QueueHandler;
