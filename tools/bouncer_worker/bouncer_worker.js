#!/usr/bin/env node

/**
 *  Copyright (C) 2015 3D Repo Ltd
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 * /

 /*
  * Bouncer Worker - The RabbitMQ Client for bouncer library
  * To run this:
  *  - ensure NodeJS/npm is installed
  *  - create config.js (use config_example.js as your base)
  *  - npm install amqplib
  *  - node bouncer_worker.js
  */

/*jshint esversion: 6 */

(() => {
	"use strict";

	const amqp = require("amqplib/callback_api");
	const conf = require("./config.js");
	const path = require("path");
	const exec = require("child_process").exec;
	const importToy = require('./importToy');
	const winston = require('winston');
	const rootModelDir = './toy';
	//Note: these error codes corresponds to error_codes.h in bouncerclient
	const ERRCODE_BOUNCER_CRASH = 12;
	const ERRCODE_PARAM_READ_FAIL = 13;
	const ERRCODE_BUNDLE_GEN_FAIL = 14;
	const softFails = [7,10,15]; //failures that should go through to generate bundle

	const logger = new (winston.Logger)({
		transports: [new (winston.transports.Console)({'timestamp': true}),
		  new (winston.transports.File)({'filename': conf.logLocation? conf.logLocation : "./bouncer_worker.log"})
		]
	});

	/**
	 * Test that the client is working and
	 * it is able to connect to the database
	 * Takes a callback function to with
	 * indication of success or failure
	 */
	function testClient(callback){
		logger.info("Checking status of client...");

		const cmd = path.normalize(conf.bouncer.path) + " " +
				conf.bouncer.dbhost + " " +
				conf.bouncer.dbport + " " +
				conf.bouncer.username + " " +
				conf.bouncer.password + " " +
				conf.aws.bucket_name + " " +
				conf.aws.bucket_region + " test";
		process.env['AWS_ACCESS_KEY_ID']= conf.aws.access_key_id;
		process.env['AWS_SECRET_ACCESS_KEY']=  conf.aws.secret_access_key;

		exec(cmd, function(error, stdout, stderr){
			if(error !== null){
				logger.error("bouncer call errored");
				logger.debug(stdout);
			}
			else{
				logger.info("bouncer call passed");
				callback();
			}
		});
	}

	/**
	 * handle queue message
	 */
	function handleMessage(cmd, rid, callback){
		// command start with importToy is handled here instead of passing it to bouncer
		if(cmd.startsWith('importToy')){

			let args = cmd.split(' ');

			let database = args[1];
			let model = args[2];
			let modelDir = args[3];
			let skipPostProcessing = args[4];
			skipPostProcessing = skipPostProcessing && JSON.parse(skipPostProcessing) || {};

			let username = database;

			let dbConfig = {
				username: conf.bouncer.username,
				password: conf.bouncer.password,
				dbhost: conf.bouncer.dbhost,
				dbport: conf.bouncer.dbport,
				writeConcern: conf.mongoimport && conf.mongoimport.writeConcern
			};

			let dir = `${rootModelDir}/${modelDir}`;

			importToy(dbConfig, dir, username, database, model, skipPostProcessing).then(() => {
				// after importing the toy regenerate the tree as well
				if(skipPostProcessing.tree){
					callback(JSON.stringify({
						value: 0,
						database: database,
						project: model
					}), true);
				} else {
					exeCommand(`genStash ${database} ${model} tree all`, rid, callback);
				}

			}).catch(err => {

				logger.error("importToy module error");
				console.log(err, err.stack);

				callback(JSON.stringify({
					value: ERRCODE_BOUNCER_CRASH,
					message: err.message,
					database: database,
					project: model
				}), true);
			});

		} else {
			exeCommand(cmd, rid, callback);
		}

	}

	function runBouncer(logDir, cmd,  callback)
	{
		let os = require('os');
		let command = "";

		if(os.platform() === "win32")
		{

			cmd = cmd.replace("/sharedData/", conf.rabbitmq.sharedDir);
			process.env['REPO_LOG_DIR']= logDir ;
			process.env['AWS_ACCESS_KEY_ID']= conf.aws.access_key_id;
			process.env['AWS_SECRET_ACCESS_KEY']=  conf.aws.secret_access_key;

			command = path.normalize(conf.bouncer.path) + " " +
				conf.bouncer.dbhost + " " +
				conf.bouncer.dbport + " " +
				conf.bouncer.username + " " +
				conf.bouncer.password + " " +
				conf.aws.bucket_name + " " +
				conf.aws.bucket_region + " " +
				cmd;

			let cmdArr = cmd.split(' ');
			if(cmdArr[0] == "import")
			{
				let fs = require('fs')
				fs.readFile(cmdArr[2], 'utf8', function (err,data) {
				  	if (err) {
						return logger.error(err);
  					}
		  			let result = data.replace("/sharedData/", conf.rabbitmq.sharedDir);

		  			fs.writeFile(cmdArr[2], result, 'utf8', function (err) {
     						if (err) return logger.error(err);
  					});
				});
			}
		}
		else
		{
			command = "REPO_LOG_DIR=" + logDir + " " +
				"AWS_ACCESS_KEY_ID=" + conf.aws.access_key_id +
				" AWS_SECRET_ACCESS_KEY=" + conf.aws.secret_access_key + " " +
				path.normalize(conf.bouncer.path) + " " +
				conf.bouncer.dbhost + " " +
				conf.bouncer.dbport + " " +
				conf.bouncer.username + " " +
				conf.bouncer.password + " " +
				conf.aws.bucket_name + " " +
				conf.aws.bucket_region + " " +
				cmd;
		}

		let cmdFile;
		let cmdDatabase;
		let cmdProject;
		let cmdArr = cmd.split(' ');
		let toyFed = false;

		// Extract database and project information from command
		try{
			switch(cmdArr[0]) {
				case "import":
					cmdFile = require(cmdArr[2]);
					cmdDatabase = cmdFile.database;
					cmdProject = cmdFile.project;
					break;
				case "genFed":
					cmdFile = require(cmdArr[1]);
					cmdDatabase = cmdFile.database;
					cmdProject = cmdFile.project;
					toyFed = cmdFile.toyFed;
					break;
				case "importToy":
					cmdDatabase = cmdArr[1];
					cmdProject = cmdArr[2];
					break;
				case "genStash":
					cmdDatabase = cmdArr[1];
					cmdProject = cmdArr[2];
					break;
				default:
					logger.error("Unexpected command: " + cmdArr[0]);
			}
		}
		catch(error) {
			logger.error(error);
			callback({value: 16}, true);
		}

		// Issue callback to indicate job is processing, but no ack as job not done
		if ("genFed" !== cmdArr[0]) {
			callback({
				status: "processing",
				database: cmdDatabase,
				project: cmdProject
			}, false);
		}

		exec(command, function(error, stdout, stderr){
			let reply = {};
			logger.debug(stdout);

			if(error !== null && error.code && softFails.indexOf(error.code) == -1){
				if(error.code)
					reply.value = error.code;
				else
					reply.value = ERRCODE_BOUNCER_CRASH;
				callback({
					value: reply.value,
					database: cmdDatabase,
					project: cmdProject
				}, true);
				logger.info("Executed command: " + command, reply);
			}
			else{
				if(error == null)
					reply.value = 0;
				else
					reply.value = error.code;
				console.log(error);
				logger.info("Executed command: " + command, reply);
				if(conf.unity && conf.unity.project && cmdArr[0] == "import")
				{
					let commandArgs = cmdFile;
					if(commandArgs && commandArgs.database && commandArgs.project)
					{

						process.env['AWS_ACCESS_KEY_ID']= conf.aws.access_key_id;
						process.env['AWS_SECRET_ACCESS_KEY']=  conf.aws.secret_access_key;
						const unityCommand = conf.unity.batPath + " " +
							conf.unity.project + " " +
							conf.bouncer.dbhost + " " +
							conf.bouncer.dbport + " " +
							conf.bouncer.username + " " +
							conf.bouncer.password + " " +
							commandArgs.database + " " +
							commandArgs.project + " " +
							conf.aws.bucket_name + " " +
							conf.aws.bucket_region + " " +
							logDir;

						logger.info("running unity command: " + unityCommand);
						exec(unityCommand, function( error, stdout, stderr){
							if(error)
							{
								reply.value = ERRCODE_BUNDLE_GEN_FAIL;
							}
							logger.info("Executed Unity command: " + unityCommand, reply);
							callback({
								value: reply.value,
								database: cmdDatabase,
								project: cmdProject
							}, true);
						});
					}
					else
					{
						logger.error("Failed to read " + cmdArr[2]);
						reply.value = ERRCODE_PARAM_READ_FAIL;
						callback({
							value: reply.value,
							database: cmdDatabase,
							project: cmdProject
						}, true);
					}
				}
				else
				{
					if(toyFed) {

						const dbConfig = {
							username: conf.bouncer.username,
							password: conf.bouncer.password,
							dbhost: conf.bouncer.dbhost,
							dbport: conf.bouncer.dbport,
							writeConcern: conf.mongoimport && conf.mongoimport.writeConcern
						};
						const dir = `${rootModelDir}/${toyFed}`;
						importToy(dbConfig, dir, cmdDatabase, cmdDatabase, cmdProject, {tree: 1}).then(()=> {
							callback({
								value: reply.value,
								database: cmdDatabase,
								project: cmdProject
							}, true);
						});
					} else {

						callback({
							value: reply.value,
							database: cmdDatabase,
							project: cmdProject
						}, true);
					}
				}
			}

		});

	}

	/**
	 * Execute the Command and provide a reply message to the callback function
	 */
	function exeCommand(cmd, rid, callback){

		let logRootDir = conf.bouncer.log_dir;

		if(logRootDir === null) {
			logRootDir = "./log";
		}

		let logDir = logRootDir + "/" +  rid.toString() + "/";

		runBouncer(logDir, cmd,
		 function(reply, sendAck){
			callback(JSON.stringify(reply), sendAck);
		});
	}

	/*
	 * @param {sendAck} sendAck - Should an acknowledgement be sent with callback (true/false)
	 */
	function listenToQueue(ch, queueName, prefetchCount)
	{
		ch.assertQueue(queueName, {durable: true});
		logger.info("Bouncer Client Queue started. Waiting for messages in %s of %s....", queueName, conf.rabbitmq.host);
		ch.prefetch(prefetchCount);
		ch.consume(queueName, function(msg){
			logger.info(" [x] Received %s from %s", msg.content.toString(), queueName);
			handleMessage(msg.content.toString(), msg.properties.correlationId, function(reply, sendAck){
				if (sendAck)
					ch.ack(msg);
				logger.info("sending to reply queue(%s): %s", conf.rabbitmq.callback_queue, reply);
				ch.publish(conf.rabbitmq.callback_queue, msg.properties.appId, new Buffer(reply),
					{correlationId: msg.properties.correlationId, appId: msg.properties.appId});
			});
		}, {noAck: false});
	}

	function connectQ(){
		amqp.connect(conf.rabbitmq.host, function(err,conn){
			if(err !== null)
			{
				logger.error("failed to establish connection to rabbit mq");
			}
			else
			{
				conn.createChannel(function(err, ch){
					ch.assertExchange(conf.rabbitmq.callback_queue, 'direct', { durable: true });
					listenToQueue(ch, conf.rabbitmq.worker_queue, conf.rabbitmq.task_prefetch || 4);
					listenToQueue(ch, conf.rabbitmq.model_queue, conf.rabbitmq.model_prefetch || 1);

				});
			}
		});
	}

	logger.info("Initialising bouncer client queue...");
	testClient(connectQ);

})();

