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
	const {
		ERRCODE_BOUNCER_CRASH,
		ERRCODE_PARAM_READ_FAIL,
		ERRCODE_BUNDLE_GEN_FAIL,
		REPOERR_ARG_FILE_FAIL,
		BOUNCER_SOFT_FAILS } = require("../constants/errorCodes");
	const { config, configPath}  = require("../lib/config");
	const { testClient, runCommand = runBouncerCommand } = require("../tasks/bouncerClient");
	const { generateAssetBundles } = require("../tasks/unityEditor");
	const { connectToQueue } = require("../lib/queueHandler");
	const logger = require("../lib/logger");


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
		const platform  = require('os').platform();
		if (platform === "win32") {
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
	}

	const extractTaskInfo = (cmdArr) => {
		const command = cmdArr[0];
		let cmdFile;
		let cmdDatabase;
		let cmdProject;
		let toyFed = false;
		let user;
		let errorCode;

		try {
			switch(command) {
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
				errorCode = REPOERR_ARG_FILE_FAIL;
			}
		} catch (error) {
			logger.error(error);
			errorCode = REPOERR_ARG_FILE_FAIL;
		}

		return {command, cmdFile, cmdDatabase, cmdProject, toyFed, user, errorCode};
	};

	async function runBouncer(logDir, cmd,  callback)
	{
		let command = path.normalize(config.bouncer.path);
		const cmdParams = [];

		cmdParams.push(configFullPath);
		win32Workaround(cmd);

		let cmdArr = cmd.split(' ');

		cmdArr.forEach((data) => cmdParams.push(data));

		// Extract database and project information from command
		const {command, cmdFile, cmdDatabase, cmdProject, toyFed, user, errorCode} = extractTaskInfo(cmdArr);
		if(errorCode) {
			callback({ value: errorCode });
			return;
		}

		if ("genFed" !== command) {
			callback({
				status: "processing",
				database: cmdDatabase,
				project: cmdProject
			});
		}

		try {
			const code = await runBouncerCommand(logDir, cmdParams);
			logger.info(`[SUCCEED] Executed command: ${command} ${cmdParams.join(" ")} `, code);

			if (command === "import") {
				await generateAssetBundles(cmdDatabase, cmdProject, logDir);
			} else if (command === "toyFed") {
				const dbConfig = {
					username: config.db.username,
					password: config.db.password,
					dbhost: config.db.dbhost,
					dbport: config.db.dbport,
					writeConcern: config.mongoimport && config.mongoimport.writeConcern
				};
				const dir = `${rootModelDir}/${toyFed}`;
				await importToy(dbConfig, dir, cmdDatabase, cmdDatabase, cmdProject, {tree: 1});
			}
			callback({
				value: code,
				database: cmdDatabase,
				project: cmdProject,
				user
			});
		} catch (code) {
			const err =  code || ERRCODE_BOUNCER_CRASH;
			callback({
				value: err,
				database: cmdDatabase,
				project: cmdProject,
				user
			});
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

