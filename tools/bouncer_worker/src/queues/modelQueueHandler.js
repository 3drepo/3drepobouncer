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

const { config } = require('../lib/config');
const { runCommand = runBouncerCommand } = require('../tasks/bouncerClient');
const { ERRCODE_BOUNCER_CRASH } = require("../constants/errorCodes");
const { generateAssetBundles } = require("../tasks/unityEditor");

const onMessageReceived = async (cmd, rid, callback) => {
	const logDir = `${config.bouncer.log_dir}/${rid.toString()}/`;
	const cmdMsg = messageDecoder(cmd);

	if(cmdMsg.errorCode) {
		callback({value: cmdMsg.errorCode});
	} else {

		callback({
			status: "processing",
			database: cmdMsg.database,
			project: cmdMsg.model
		});

		const message = await processModel(cmdMsg, logDir);
		callback(JSON.stringify(message));
	}
}

const processModel = async ({database, model, cmdParams}) => {
	const returnMessage = {
		value: ERRCODE_OK,
		database: database,
		project: model
	};
	try {
		returnMessage.value = await runBouncerCommand(logDir, cmdParams);
		await generateAssetBundles(database, cmdProject, logDir);
	} catch(err) {
		logger.error(`Import model error: ${err.message || err}`);
		returnMessage.value = err || ERRCODE_BOUNCER_CRASH;
	}

	return returnMessage;
}



module.exports = { onMessageReceived };
