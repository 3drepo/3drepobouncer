/**
 * Copyright (C) 2021 3D Repo Ltd
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
const { config } = require('../lib/config');
const logger = require('../lib/logger');

const configSpecified = (parent, key, logLabel) => {
	if (!config[parent] || !config[parent][key]) {
		logger.error(`${parent}.${key} is not specified!`, logLabel);
		return false;
	}
	return true;
};

const pathExists = (parent, key, logLabel) => {
	const filePath = config[parent][key];
	if (!fs.existsSync(filePath)) {
		logger.error(`${filePath} (taken from ${parent}.${key}) not found.`, logLabel);
		return false;
	}
	return true;
};

const queueSpecified = (queue, logLabel) => configSpecified('rabbitmq', queue, logLabel);

const CommonQueueUtils = {};

CommonQueueUtils.callbackQueueSpecified = (logLabel) => queueSpecified('callback_queue', logLabel);

CommonQueueUtils.logDirExists = (logLabel) => configSpecified('logging', 'taskLogDir', logLabel)
											&& pathExists('logging', 'taskLogDir', logLabel);
CommonQueueUtils.sharedDirExists = (logLabel) => configSpecified('rabbitmq', 'sharedDir', logLabel)
											&& pathExists('rabbitmq', 'sharedDir', logLabel);

module.exports = CommonQueueUtils;
