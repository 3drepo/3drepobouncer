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

module.exports = {
	ERRCODE_OK: 0,
	ERRCODE_UNKNOWN_ERROR: 4,
	ERRCODE_BOUNCER_CRASH: 12,
	ERRCODE_PARAM_READ_FAIL: 13,
	ERRCODE_BUNDLE_GEN_FAIL: 14,
	ERRCODE_ARG_FILE_FAIL: 16,
	ERRCODE_TIMEOUT: 29,
	ERRCODE_TOY_IMPORT_FAILED: 33,
	ERRCODE_UNITY_LICENCE_INVALID: 35,
	BOUNCER_SOFT_FAILS: [7, 10, 15], // failures that should go through to generate
};
