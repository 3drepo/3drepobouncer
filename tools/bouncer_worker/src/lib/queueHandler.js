const amqp = require('amqplib');
const { rabbitmq } = require('./config').config;
const logger = require('./logger');
const JobQHandler = require('../queues/jobQueueHandler');
const ModelQHandler = require('../queues/modelQueueHandler');
const UnityQHandler = require('../queues/unityQueueHandler');

let connClosed = false;
let retry = 0;

const QueueHandler = {};

const reconnect = (listenToJobQueue, listenToModelQueue) => {
	const maxRetries = rabbitmq.maxRetries || 3;
	if (++retry <= maxRetries) {
		logger.error(`[AMQP] Trying to reconnect [${retry}/${maxRetries}]...`);
		QueueHandler.connectToQueue(listenToJobQueue, listenToModelQueue);
	} else {
		logger.error('[AMQP] Retries exhausted');
		// eslint-disable-next-line no-process-exit
		process.exit(-1);
	}
};

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

QueueHandler.connectToQueue = async (listenToJobQueue, listenToModelQueue, listenToUnityQueue) => {
	try {
		const conn = await amqp.connect(rabbitmq.host);
		retry = 0;
		connClosed = false;
		logger.info('[AMQP] Connected! Creating channel...');

		conn.on('close', () => {
			if (!connClosed) {
				// this can be called more than once for some reason. Use a boolean to distinguish first timers.
				connClosed = true;
				logger.error('[AMQP] connection closed.');
				reconnect();
			}
		});

		conn.on('error', (err) => {
			logger.error(`[AMQP] connection error: ${err.message}`);
		});

		await establishChannel(conn, listenToJobQueue, listenToModelQueue, listenToUnityQueue);
	} catch (err) {
		logger.error(`[AMQP] failed to establish connection to rabbit mq: ${err}.`);
		reconnect(listenToJobQueue, listenToModelQueue);
	}
};

module.exports = QueueHandler;
