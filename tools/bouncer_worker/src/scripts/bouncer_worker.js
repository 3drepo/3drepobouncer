#!/usr/bin/env node

/**
 *	Copyright (C) 2015 3D Repo Ltd
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU Affero General Public License as
 *	published by the Free Software Foundation, either version 3 of the
 *	License, or (at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU Affero General Public License for more details.
 *
 *	You should have received a copy of the GNU Affero General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>
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
	const path = require("path");
	const run = require("../lib/runCommand");
	const { ERRCODE_BOUNCER_CRASH, ERRCODE_PARAM_READ_FAIL, ERRCODE_BUNDLE_GEN_FAIL, BOUNCER_SOFT_FAILS } = require("../constants/errorCodes");
	const { config, configPath}  = require("../lib/config");
	const { testClient, runCommand = runBouncerCommand } = require("../tasks/bouncerClient");
	const { connectToQueue } = require("../lib/queueHandler");
	const logger = require("../lib/logger");


	const amqp = require("amqplib");
	const importToy = require('../toy/importToy');
	const rootModelDir = './toy';
	let retry = 0;
	let connClosed = false;



	/**
	 * handle queue message
	 */
	async function handleMessage(cmd, rid, callback){
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
				username: config.db.username,
				password: config.db.password,
				dbhost: config.db.dbhost,
				dbport: config.db.dbport,
				writeConcern: config.mongoimport && config.mongoimport.writeConcern
			};

			let dir = `${rootModelDir}/${modelDir}`;

			try {
			await importToy(dbConfig, dir, username, database, model, skipPostProcessing);
				// after importing the toy regenerate the tree as well
				if(skipPostProcessing.tree){
					callback(JSON.stringify({
						value: 0,
						database: database,
						project: model
					}));
				} else {
					exeCommand(`genStash ${database} ${model} tree all`, rid, callback);
				}

			}
			catch(err) {
				logger.error("importToy module error");
				callback(JSON.stringify({
					value: ERRCODE_BOUNCER_CRASH,
					message: err.message,
					database: database,
					project: model
				}));
			};

		} else {
			exeCommand(cmd, rid, callback);
		}

	}

	const win32Workaround = (cmd) => {
		// messages coming in assume the sharedData is stored in a specific linux style directory
		// we need to do a find/replace to make it use rabbitmq sharedDir instead
		cmd = cmd.replace("/sharedData/", config.rabbitmq.sharedDir);
		let cmdArr = cmd.split(' ');
		if(cmdArr[0] == "import")
		{
			const fs = require('fs')
			const data = fs.readFileSync(cmdArr[2], 'utf8');
			const result = data.replace("/sharedData/", config.rabbitmq.sharedDir);
			fs.writeFileSync(cmdArr[2], result, 'utf8');
		}
	}


	async function runBouncer(logDir, cmd,  callback)
	{
		const os = require('os');
		let command = path.normalize(config.bouncer.path);
		const cmdParams = [];

		cmdParams.push(configFullPath);
		win32Workaround(cmd);

		cmd.split(' ').forEach((data) => cmdParams.push(data));

		let cmdFile;
		let cmdDatabase;
		let cmdProject;
		let cmdArr = cmd.split(' ');
		let toyFed = false;
		let user = "";

		// Extract database and project information from command
		try{
			switch(cmdArr[0]) {
				case "import":
					cmdFile = require(cmdArr[2]);
					cmdDatabase = cmdFile.database;
					cmdProject = cmdFile.project;
					user = cmdFile.owner;
					break;
				case "genFed":
					cmdFile = require(cmdArr[1]);
					cmdDatabase = cmdFile.database;
					cmdProject = cmdFile.project;
					toyFed = cmdFile.toyFed;
					user = cmdFile.owner;
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

		try {

			const code = await runBouncerCommand(logDir, cmdParams);
			logger.info(`[SUCCEED] Executed command: ${command} ${cmdParams.join(" ")} `, code);
			if(config.unity && config.unity.project && cmdArr[0] == "import")
			{
				let commandArgs = cmdFile;
				if(commandArgs && commandArgs.database && commandArgs.project)
				{
					let awsBucketName = "undefined";
					let awsBucketRegion = "undefined";

					if (config.aws)
					{
						process.env['AWS_ACCESS_KEY_ID'] = config.aws.access_key_id;
						process.env['AWS_SECRET_ACCESS_KEY'] =	config.aws.secret_access_key;
						awsBucketName = config.aws.bucket_name;
						awsBucketRegion = config.aws.bucket_region;
					}

					const unityCommand = config.unity.batPath;
					const unityCmdParams = [
							config.unity.project,
							configFullPath,
							commandArgs.database,
							commandArgs.project,
							logDir
					];

					logger.info(`Running unity command: ${unityCommand} ${unityCmdParams.join(" ")}`);

					try {
						const unityCode = await run(unityCommand, unityCmdParams);

						logger.info(`[SUCCESS] Executed unity command: ${unityCommand} ${unityCmdParams.join(" ")}`, unityCode);
						callback({
							value: code,
							database: cmdDatabase,
							project: cmdProject,
							user
						});
					} catch(unityCode) {
						logger.info(`[FAILED] Executed unity command: ${unityCommand} ${unityCmdParams.join(" ")}`, unityCode);
						callback({
							value: ERRCODE_BUNDLE_GEN_FAIL,
							database: cmdDatabase,
							project: cmdProject,
							user
						}, true);
					}
				}
				else
				{
					logger.error("Failed to read " + cmdArr[2]);
					callback({
						value: ERRCODE_PARAM_READ_FAIL,
						database: cmdDatabase,
						project: cmdProject,
						user
					}, true);
				}
			}
			else
			{
				if(toyFed) {

					const dbConfig = {
						username: config.db.username,
						password: config.db.password,
						dbhost: config.db.dbhost,
						dbport: config.db.dbport,
						writeConcern: config.mongoimport && config.mongoimport.writeConcern
					};
					const dir = `${rootModelDir}/${toyFed}`;
					importToy(dbConfig, dir, cmdDatabase, cmdDatabase, cmdProject, {tree: 1}).then(()=> {
						callback({
							value: code,
							database: cmdDatabase,
							project: cmdProject,
							user
						}, true);
					});
				} else {

					callback({
						value: code,
						database: cmdDatabase,
						project: cmdProject,
						user
					}, true);
				}
			}

		} catch(code) {
			const err =  code || ERRCODE_BOUNCER_CRASH;
			callback({
				value: err,
				database: cmdDatabase,
				project: cmdProject,
				user
			}, true);
			logger.error(`[FAILED] Executed command: ${command} ${cmdParams.join(" ")}`, err);
		}

	}

	/**
	 * Execute the Command and provide a reply message to the callback function
	 */
	async function exeCommand(cmd, rid, callback){

		let logRootDir = config.bouncer.log_dir;

		let logDir = logRootDir + "/" +  rid.toString() + "/";

		await runBouncer(logDir, cmd,
			(reply) => callback(JSON.stringify(reply)));
	}

	logger.info("Initialising bouncer client queue...");
	if(config.hasOwnProperty("umask")) {
		logger.info("Setting umask: " + config.umask);
		process.umask(config.umask);
	}
	testClient().then(() => connectToQueue(handleMessage, handleMessage));

})();

