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

const fs = require('fs');
const { configPath, replaceSharedDirTag } = require('./config');
const { ERRCODE_ARG_FILE_FAIL } = require('../constants/errorCodes');
const logger = require('./logger');

const replaceSharedDirPlaceHolder = (command) => {
	// messages coming in has a placeholder for $SHARED_SPACE.
	// we need to do a find/replace to make it use rabbitmq sharedDir instead
	let cmd = command;
	const cmdArr = cmd.split(/\s+/);
	if (cmdArr[0] === 'import') {
		cmd = replaceSharedDirTag(cmd);
		cmdArr[2] = replaceSharedDirTag(cmdArr[2]);
		const result = replaceSharedDirTag(fs.readFileSync(cmdArr[2], 'utf8'));
		fs.writeFileSync(cmdArr[2], result, 'utf8');
	} else if (cmdArr[0] === 'processDrawing') {
		cmd = replaceSharedDirTag(cmd);
		cmdArr[1] = replaceSharedDirTag(cmdArr[1]);
		const result = replaceSharedDirTag(fs.readFileSync(cmdArr[1], 'utf8'));
		fs.writeFileSync(cmdArr[1], result, 'utf8');
	}
	return cmd;
};

const messageDecoder = (cmd) => {
	let res = { };
	try {
		const args = replaceSharedDirPlaceHolder(cmd).split(/\s+/);
		res = { command: args[0] };
		switch (args[0]) {
			case 'import':
				{
					// eslint-disable-next-line
					const cmdFile = require(args[2]);
					res = {
						cmdParams: [configPath, ...args],
						teamspace: cmdFile.teamspace,
						container: cmdFile.container,
						user: cmdFile.owner,
						file: cmdFile.file,
						...res,
					};
				}
				break;
			case 'processDrawing':
				{
					// eslint-disable-next-line
					const cmdFile = require(args[1]);
					res = {
						cmdParams: [configPath, ...args],
						teamspace: cmdFile.teamspace,
						drawing: cmdFile.drawing,
						user: cmdFile.owner,
						file: cmdFile.file,
						format: cmdFile.format,
						size: cmdFile.size,
						...res,
					};
				}
				break;
			case 'genFed':
				{
					// eslint-disable-next-line
					const cmdFile = require(args[1]);
					res = {
						cmdParams: [configPath, ...args],
						teamspace: cmdFile.teamspace,
						federation: cmdFile.federation,
						user: cmdFile.owner,
						...res,
					};
				}
				break;
			case 'processClash':
				res = {
					cmdParams: [configPath],
					teamspace: args[1],
					project: args[2],
					configFile: args[3],
					...res,
				};
				break;
			case 'genStash':
				res = {
					cmdParams: [configPath, ...args],
					teamspace: args[1],
					container: args[2],
					...res,
				};
				break;
			default:
				res = { errorCode: ERRCODE_ARG_FILE_FAIL };
		}
	} catch (err) {
		logger.error(`Failed to parse message: ${err.message || err}`);
		res = { errorCode: ERRCODE_ARG_FILE_FAIL };
	}
	return res;
};

module.exports = { messageDecoder };
