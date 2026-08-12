/**
 * Copyright (C) 2026 3D Repo Ltd
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

// Keep these random generators separate to the helpers that require
// importing config, as some test files cannot have config loaded too
// early.

const { randomBytes, randomUUID } = require('node:crypto');
const path = require('path');

const generateRandomString = (length = 8) => randomBytes(length).toString('hex');

const Random = {};

Random.generateUUIDString = () => randomUUID().toString();

Random.generateRandomString = generateRandomString;

Random.generateRandomPath = () => path.normalize(`${generateRandomString()}/${generateRandomString()}}`);

Random.generateRandomSentence = () => {
	let sentence = '';
	for (let i = 0; i < 3 + (Math.random() * 7); i++) {
		// eslint-disable-next-line prefer-template
		sentence += generateRandomString() + ' ';
	}
	return sentence;
};

Random.generateRandomFilepath = () => {
	let filepath = '';
	for (let i = 0; i < 1 + (Math.random() * 3); i++) {
		// eslint-disable-next-line prefer-template
		filepath += '/' + generateRandomString();
	}
	return `${filepath}.${generateRandomString(3)}`;
};

Random.generateRandomSize = () => Math.floor(Math.random() * 1000) + 2;

Random.generateRandomDate = () => new Date(+(new Date()) - Math.floor(Math.random() * 10000000000));

module.exports = Random;
