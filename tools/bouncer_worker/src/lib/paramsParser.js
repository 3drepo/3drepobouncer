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

const yargs = require('yargs/yargs');
const { hideBin } = require('yargs/helpers');

const ParamsParser = {};

ParamsParser.parseParameters = () => {
	const args = yargs(hideBin(process.argv));
	return args.option('config', {
		describe: 'specify the path to a custom configuration file (default: config.json at root level)',
		string: true,
	}).option('exitAfter', {
		describe: 'exit upon finishing the defined amount of tasks. Queue must also specified.',
		number: true,
	}).option('queue', {
		describe: 'specify which queue to run on [job|model|unity]',
		choice: ['job', 'model', 'unity'],
		string: true,
	}).help().argv;
};

module.exports = ParamsParser;
