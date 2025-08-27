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
const PDF2SVG = 'pdf2svg';

ImageProcessing.testImageClient = async () => {
	try {
		await run(PDF2SVG, [], { label: 'INIT' });
	} catch (err) {
		throw new Error(`Failed to call ${PDF2SVG}: ${err?.message}`);
	}
};

ImageProcessing.generateSVG = async (file, output, taskInfo) => {
	const retVal = await run(
		PDF2SVG,
		[file, output, 'all'],
		{ label: 'PDF2SVG' },
		taskInfo,
	);
	if (retVal !== ERRCODE_OK) return retVal;

	try {
		// Not using inkscape anymore, but probably still sensible to check if the
		// file exists with non 0 values.
		await access(output);
		return ERRCODE_OK;
	} catch {
		throw ERRCODE_IMAGE_PROCESSING_FAILED;
	}
};

module.exports = ImageProcessing;
