/**
 * Copyright (C) 2024 3D Repo Ltd
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

const { access } = require('fs/promises');
const run = require('../lib/runCommand');
const {
	ERRCODE_OK,
	ERRCODE_IMAGE_PROCESSING_FAILED,
} = require('../constants/errorCodes');

const ImageProcessing = {};

ImageProcessing.generateSVG = async (file, output, taskInfo) => {
	const retVal = await run(
		'inkscape',
		['--export-type="svg"', file, '-o', output, '-n', '1', '-D'],
		{ logLabel: 'INKSCAPE' },
		taskInfo,
	);
	if (retVal !== ERRCODE_OK) return retVal;

	try {
		// Inkscape doesn't always exit with non 0 values, so we need to check
		// if the file exists, see https://gitlab.com/inkscape/inkscape/-/issues/270
		await access(output);
		return ERRCODE_OK;
	} catch {
		throw ERRCODE_IMAGE_PROCESSING_FAILED;
	}
};

module.exports = ImageProcessing;
