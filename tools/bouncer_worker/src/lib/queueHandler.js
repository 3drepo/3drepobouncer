const amqp = require("amqplib");
const { rabbitmq } = require("./config").config;
const logger = require("../lib/logger");
const JobQHandler = require("../queues/jobQueueHandler");
const ModelQHandler = require("../queues/modelQueueHandler");

let connClosed = false;
let retry = 0;

const reconnect = (listenToJobQueue, listenToModelQueue) => {
	const maxRetries = rabbitmq.maxRetries || 3;
	if(++retry <= maxRetries) {
		logger.error(`[AMQP] Trying to reconnect [${retry}/${maxRetries}]...`);
		QueueHandler.connectToQueue(listenToJobQueue, listenToModelQueue);
	} else {
		logger.error("[AMQP] Retries exhausted");
		process.exit(-1);
	}
}

const establishChannel = async (conn, listenToJobQueue, listenToModelQueue) => {
	const channel = await conn.createChannel();
	channel.assertQueue(rabbitmq.callback_queue, { durable: true });
	rabbitmq.worker_queue && listenToJobQueue &&
		listenToQueue(channel, rabbitmq.worker_queue, rabbitmq.task_prefetch, JobQHandler.onMessageReceived);
	rabbitmq.model_queue && listenToModelQueue &&
		listenToQueue(channel, rabbitmq.model_queue, rabbitmq.model_prefetch, ModelQHandler.onMessageReceived);
}

const listenToQueue = (channel, queueName, prefetchCount, callback) => {
	channel.assertQueue(queueName, {durable: true});
	logger.info("Bouncer Client Queue started. Waiting for messages in %s of %s....", queueName, rabbitmq.host);
	channel.prefetch(prefetchCount);
	channel.consume(queueName, async (msg) => {
		logger.info(" [x] Received %s from %s", msg.content.toString(), queueName);
		await callback(msg.content.toString(), msg.properties.correlationId, (reply) => {
			logger.info("sending to reply queue(%s): %s", rabbitmq.callback_queue, reply);
			channel.sendToQueue(rabbitmq.callback_queue, new Buffer.from(reply),
				{correlationId: msg.properties.correlationId, appId: msg.properties.appId});
		});
		channel.ack(msg);
	}, {noAck: false});
}

const QueueHandler = {};

QueueHandler.connectToQueue = async (listenToJobQueue, listenToModelQueue) => {
	try {
		const conn = await amqp.connect(rabbitmq.host);
		retry = 0;
		connClosed = false;
		logger.info("[AMQP] Connected! Creating channel...");

		conn.on("close", () => {
			if(!connClosed) {
				//this can be called more than once for some reason. Use a boolean to distinguish first timers.
				connClosed = true;
				logger.error("[AMQP] connection closed.");
				reconnect();
			}
		});

		conn.on("error", (err)	=> {
			logger.error("[AMQP] connection error: " + err.message);
		});

		await establishChannel(conn, listenToJobQueue, listenToModelQueue);
	}
	catch(err) {
		logger.error(`[AMQP] failed to establish connection to rabbit mq: ${err}.`);
		reconnect(listenToJobQueue, listenToModelQueue);
	}

};

module.exports = QueueHandler;
