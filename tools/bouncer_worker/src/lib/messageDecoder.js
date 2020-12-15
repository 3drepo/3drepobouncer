/**
 *	Copyright (C) 2020 3D Repo Ltd
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
 */

"use strict"
const { config, configPath}  = require("../lib/config");
const { ERRCODE_ARG_FILE_FAIL } = require("../constants/errorCodes");
const path = require('path');

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

const getBouncerParams = (cmd, args) => {
	win32Workaround(cmd);
	return [configPath, ...args];
}

const messageDecoder = (cmd) => {
	const args = cmd.split(' ');
	let	res = { command: args[0] };

	switch(args[0]) {
		case "importToy" :
			res = {
				command: args[0],
				database: args[1],
				model: args[2],
				toyModelID: args[3],
				skipPostProcessing: args[4] && JSON.parse(args[4]) || {}
			};
			break;
		case "import" :
			{
				const cmdFile = require(args[2]);
				res = {
					cmdParams : getBouncerParams(cmd, args),
					database : cmdFile.database,
					model : cmdFile.project,
					user: cmdFile.owner,
					...res
				};
			}
			break;
		case "genFed" :
			{
				const cmdFile = require(args[1]);
				res = {
					cmdParams : getBouncerParams(cmd, args),
					database : cmdFile.database,
					model : cmdFile.project,
					toyFed: cmdFile.toyFed,
					user: cmdFile.owner,
					...res
				};
			}
			break;
		case "genStash" :
			res = {
				cmdParams : getBouncerParams(cmd, args),
				database: args[1],
				model: args[2],
				...res
			};
			break;
		default:
			res = { errorCode: ERRCODE_ARG_FILE_FAIL };
	}
	return res;
};


module.exports = { messageDecoder };
