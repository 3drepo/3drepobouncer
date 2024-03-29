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

const winston = require('winston');
const { logLevel, noColors, jsonOutput, workerLogPath } = require('./config').config.logging;

const stringFormat = ({ level, message, label, timestamp }) => `${timestamp} [${level}] [${label || 'APP'}] ${message}`;

const getTransporters = () => {
	const transporters = [
		new (winston.transports.Console)({ timestamp: true, level: logLevel }),
	];

	if (workerLogPath) {
		transporters.push(new (winston.transports.File)({ filename: workerLogPath, timestamp: true, level: logLevel }));
	}

	return transporters;
};

const logger = () => {
	let formats;

	if (jsonOutput) {
		formats = winston.format.combine(
			winston.format.timestamp(),
			winston.format.align(),
			winston.format.json(),
		);
	} else if (noColors) {
		formats = winston.format.combine(
			winston.format.timestamp(),
			winston.format.align(),
			winston.format.printf(stringFormat),
		);
	} else {
		formats = winston.format.combine(
			winston.format.timestamp(),
			winston.format.colorize(),
			winston.format.align(),
			winston.format.printf(stringFormat),
		);
	}

	return winston.createLogger({
		transports: getTransporters(),
		format: formats,
	});
};

module.exports = logger();
