const assert = require('node:assert');
const test = require('node:test');
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
* Bouncer worker must be running for these tests.
*/

// Use the same config as bouncer_worker proper
const { config } = require('../src/lib/config');

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

	// Wait for callback
	{
		const channel = await connection.createChannel();

		await channel.assertQueue(callbackq);
		await channel.prefetch(1);

		await new Promise((resolve) => {
			channel.consume(callbackq, (message) => {
				const content = JSON.parse(message.content.toString());

				assert.equal(message.properties.correlationId, correlationId);
				assert.equal(content.value, 0);
				assert.equal(content.results, path.join(clashConfigDirectory, 'results.json'));
				assert.equal(fs.existsSync(content.results), true);
				assert.equal(content.project, project);
				assert.equal(content.teamspace, teamspace);

				channel.ack(message);
				resolve();
			});
		});

		await channel.close();
	}

	await connection.close();
});
