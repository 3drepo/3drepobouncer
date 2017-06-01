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
	const toyProjectDir = './toy';
	/**
	 * Test that the client is working and
	 * it is able to connect to the database
	 * Takes a callback function to with
	 * indication of success or failure
	 */
	function testClient(callback){
		console.log("Checking status of client...");

		exec(path.normalize(conf.bouncer.path) + " " + conf.bouncer.dbhost + " " + conf.bouncer.dbport + " " + conf.bouncer.username + " " + conf.bouncer.password + " test", function(error, stdout, stderr){
			if(error !== null){
				console.log("bouncer call errored");
				console.log(stdout);
			}
			else{
				console.log("bouncer call passed");
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
			let project = args[2];
			let username = database;

			let dbConfig = {
				username: conf.bouncer.username,
				password: conf.bouncer.password,
				dbhost: conf.bouncer.dbhost,
				dbport: conf.bouncer.dbport,
				writeConcern: conf.mongoimport && conf.mongoimport.writeConcern
			};

			importToy(dbConfig, toyProjectDir, username, database, project).then(() => {
				// after importing the toy regenerate the tree as well
				exeCommand(`genStash ${database} ${project} tree`, rid, callback);
			}).catch(err => {

				console.log("importToy module error", err, err.stack);

				callback(JSON.stringify({
					value: 12,
					message: err.message
				}));
			});

		} else {
			exeCommand(cmd, rid, callback);
		}

	} 

	function runBouncer(logDir, cmd,  callback)
	{
		let command = "REPO_LOG_DIR=" + logDir + " " +path.normalize(conf.bouncer.path) + " " + conf.bouncer.dbhost + " " + conf.bouncer.dbport + " " + conf.bouncer.username + " " + conf.bouncer.password + " " + cmd;
		exec(command, function(error, stdout, stderr){
			let reply = {};

			if(error !== null){
				if(error.code)
					reply.value = error.code;
				else
					reply.value = 12;
				callback(reply);
				console.log("Executed command: " + command, reply);
			}
			else{
				reply.value = 0;
				console.log("Executed command: " + command, reply);
				let cmdArr = cmd.split(' ');
				if(cmdArr[0] == "import")
				{
					let fs = require("fs");
					let commandArgs = fs.readFilesync(cmdArr[1]);
					if(commandArgs && commandArgs.database && commandArgs.project)
					{		

						let unityCommand = conf.unity.exePath + " " + conf.unity.project + " " + conf.bouncer.dbhost + " " + conf.bouncer.dbport + " " + conf.bouncer.username + " " + conf.bouncer.passwordi + " " + commandArgs.database + " " commandArgs.project + " " + logDir;
						exec(unityCommand, function( error, stdout, stderr){
							if(error && error.code)
							{
								reply.value = 14;
							}
							console.log("Executed Unity command: " + unityCommand, reply);
							callback(reply);
						});
					}
					else
					{
						console.log("Failed to read " + cmdArr[1]);
						reply.value = 13;
						callback(reply);
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
		 function(reply){
			callback(JSON.stringify(reply));
		});
	}

	function connectQ(){
		amqp.connect(conf.rabbitmq.host, function(err,conn){
			if(err !== null)
			{
				console.log("failed to establish connection to rabbit mq");
			}
			else
			{
				conn.createChannel(function(err, ch){
					ch.assertExchange(conf.rabbitmq.callback_queue, 'direct', { durable: true });

					ch.assertQueue(conf.rabbitmq.worker_queue, {durable: true});
					console.log("Bouncer Client Queue started. Waiting for messages in %s of %s....", conf.rabbitmq.worker_queue, conf.rabbitmq.host);
					ch.prefetch(1);
					ch.consume(conf.rabbitmq.worker_queue, function(msg){
						console.log(" [x] Received %s", msg.content.toString());

						handleMessage(msg.content.toString(), msg.properties.correlationId, function(reply){
							console.log("sending to reply queue(%s): %s", conf.rabbitmq.callback_queue, reply);
							ch.publish(conf.rabbitmq.callback_queue, msg.properties.appId, new Buffer(reply),
								{correlationId: msg.properties.correlationId, appId: msg.properties.appId});
						});
					}, {noAck: true});
				});
			}
		});
	}

	console.log("Initialising bouncer client queue...");
	testClient(connectQ);

})();

