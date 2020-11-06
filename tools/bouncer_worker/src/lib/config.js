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

const path = require("path");
const fs = require("fs");

const configFullPath = path.resolve(__dirname, "../../config.json");

const parseConfig = () => {
	try {
		return JSON.parse(fs.readFileSync(configFullPath))
	} catch (err) {
		//can't use logger -> circular dependency.
		console.error("Failed to parse config file:", err);
		process.exit(1);
	}
}


module.exports = {
	config: parseConfig(),
	configPath: configFullPath
};
