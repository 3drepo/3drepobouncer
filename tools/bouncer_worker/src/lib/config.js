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

const path = require('path');
const fs = require('fs');

const configFullPath = path.resolve(__dirname, '../../config.json');

const applyDefaultValuesIfUndefined = config => {
	config.timeoutMS =	config.timeoutMS || 180 * 60 * 1000;
	config.rabbitmq.maxRetries = config.rabbitmq.maxRetries || 3;
	config.rabbitmq.task_prefetch = config.rabbitmq.task_prefetch || 4;
	config.rabbitmq.model_prefetch = config.rabbitmq.model_prefetch || 1;
	config.bouncer.log_dir = config.bouncer.log_dir || '.log';
	config.mongoimport = config.mongoimport || {};
	config.mongoimport.writeConcern = config.mongoimport.writeConcern || { w: 1 };
	config.toyModelDir = config.toyModelDir || path.resolve(__dirname, '../../toy');
};

const parseConfig = () => {
	try {
		const config = JSON.parse(fs.readFileSync(configFullPath));
		applyDefaultValuesIfUndefined(config);
		if (config.hasOwnProperty('umask')) {
			logger.info(`Setting umask: ${config.umask}`);
			process.umask(config.umask);
		}
		return config;
	} catch (err) {
		// can't use logger -> circular dependency.
		console.error('Failed to parse config file:', err);
		process.exit(1);
	}
};

module.exports = {
	config: parseConfig(),
	configPath: configFullPath,
};
