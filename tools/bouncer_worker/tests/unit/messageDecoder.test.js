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

const { describe, test, afterEach } = require('node:test');
const assert = require('node:assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { createRequire } = require('node:module');

const moduleRequire = createRequire(__filename);

const messageDecoderPath = require.resolve('../../src/lib/messageDecoder');
const configPath = require.resolve('../../src/lib/config');
const loggerPath = require.resolve('../../src/lib/logger');
const errorCodesPath = require.resolve('../../src/constants/errorCodes');

const { ERRCODE_ARG_FILE_FAIL } = moduleRequire(errorCodesPath);

const jsonModulesToClear = [];

const clearModuleCache = () => {
	delete require.cache[messageDecoderPath];
	delete require.cache[configPath];
	delete require.cache[loggerPath];
	jsonModulesToClear.splice(0).forEach((jsonPath) => {
		try {
			const resolved = require.resolve(jsonPath);
			delete require.cache[resolved];
		} catch (_err) {
			// ignore missing path
		}
	});
};

const loadMessageDecoderWithMocks = ({ sharedDir, configValue = 'C:/fake/config.json' } = {}) => {
	const logs = {
		error: [],
	};

	const replaceSharedDirTag = (input) => input.replace('$SHARED_SPACE', sharedDir);

	require.cache[configPath] = {
		id: configPath,
		filename: configPath,
		loaded: true,
		exports: {
			configPath: configValue,
			replaceSharedDirTag,
		},
	};

	require.cache[loggerPath] = {
		id: loggerPath,
		filename: loggerPath,
		loaded: true,
		exports: {
			error: (...args) => logs.error.push(args),
		},
	};

	return {
		messageDecoder: moduleRequire(messageDecoderPath).messageDecoder,
		logs,
	};
};

const toCommandPath = (absolutePath) => absolutePath.replace(/\\/g, '/');

describe(__filename, () => {
	afterEach(clearModuleCache);

	test('decodes import command and rewrites shared-space placeholders in config file', () => {
		const sharedDir = toCommandPath(fs.mkdtempSync(path.join(os.tmpdir(), 'decoder-import-')));
		const cmdFilePath = path.join(sharedDir, 'import.json');
		const cmdFilePathCmd = toCommandPath(cmdFilePath);
		const modelPath = '$SHARED_SPACE/model.ifc';
		const cmdFile = {
			teamspace: 'team-a',
			container: 'container-a',
			owner: 'user-a',
			file: modelPath,
			extra: '$SHARED_SPACE/extra',
		};

		fs.writeFileSync(cmdFilePath, JSON.stringify(cmdFile));
		jsonModulesToClear.push(cmdFilePathCmd);

		const { messageDecoder } = loadMessageDecoderWithMocks({
			sharedDir,
			configValue: 'C:/repo/local.json',
		});

		const result = messageDecoder('import -f $SHARED_SPACE/import.json');

		assert.deepEqual(result.cmdParams, ['C:/repo/local.json', 'import', '-f', cmdFilePathCmd]);
		assert.equal(result.command, 'import');
		assert.equal(result.teamspace, 'team-a');
		assert.equal(result.container, 'container-a');
		assert.equal(result.user, 'user-a');
		assert.equal(result.file, `${toCommandPath(sharedDir)}/model.ifc`);

		const rewritten = JSON.parse(fs.readFileSync(cmdFilePath, 'utf8'));
		assert.equal(rewritten.file, `${toCommandPath(sharedDir)}/model.ifc`);
		assert.equal(rewritten.extra, cmdFile.extra);
	});

	test('decodes processDrawing command and rewrites placeholders', () => {
		const sharedDir = toCommandPath(fs.mkdtempSync(path.join(os.tmpdir(), 'decoder-drawing-')));
		const cmdFilePath = path.join(sharedDir, 'drawing.json');
		const cmdFilePathCmd = toCommandPath(cmdFilePath);

		fs.writeFileSync(cmdFilePath, JSON.stringify({
			teamspace: 'team-d',
			drawing: 'drawing-1',
			owner: 'user-d',
			file: '$SHARED_SPACE/input.png',
			format: 'png',
			size: 123,
		}));
		jsonModulesToClear.push(cmdFilePathCmd);

		const { messageDecoder } = loadMessageDecoderWithMocks({ sharedDir });
		const result = messageDecoder('processDrawing $SHARED_SPACE/drawing.json');

		assert.deepEqual(result.cmdParams, ['C:/fake/config.json', 'processDrawing', cmdFilePathCmd]);
		assert.equal(result.command, 'processDrawing');
		assert.equal(result.teamspace, 'team-d');
		assert.equal(result.drawing, 'drawing-1');
		assert.equal(result.user, 'user-d');
		assert.equal(result.format, 'png');
		assert.equal(result.size, 123);

		const rewritten = JSON.parse(fs.readFileSync(cmdFilePath, 'utf8'));
		assert.equal(rewritten.file, `${toCommandPath(sharedDir)}/input.png`);
	});

	test('decodes processClash command', () => {
		const sharedDir = fs.mkdtempSync(path.join(os.tmpdir(), 'decoder-clash-'));
		const { messageDecoder } = loadMessageDecoderWithMocks({ sharedDir, configValue: '/tmp/cfg.json' });

		const result = messageDecoder('processClash ts-1 project-1 /tmp/clash.json');

		assert.equal(result.command, 'processClash');
		assert.deepEqual(result.cmdParams, ['/tmp/cfg.json']);
		assert.equal(result.teamspace, 'ts-1');
		assert.equal(result.project, 'project-1');
		assert.equal(result.configFile, '/tmp/clash.json');
	});

	test('decodes genStash command', () => {
		const sharedDir = fs.mkdtempSync(path.join(os.tmpdir(), 'decoder-stash-'));
		const { messageDecoder } = loadMessageDecoderWithMocks({ sharedDir, configValue: '/tmp/cfg.json' });

		const result = messageDecoder('genStash ts-1 model-1 full all');

		assert.equal(result.command, 'genStash');
		assert.deepEqual(result.cmdParams, ['/tmp/cfg.json', 'genStash', 'ts-1', 'model-1', 'full', 'all']);
		assert.equal(result.teamspace, 'ts-1');
		assert.equal(result.container, 'model-1');
	});

	test('returns ERRCODE_ARG_FILE_FAIL for unknown command', () => {
		const sharedDir = fs.mkdtempSync(path.join(os.tmpdir(), 'decoder-unknown-'));
		const { messageDecoder } = loadMessageDecoderWithMocks({ sharedDir });

		const result = messageDecoder('doSomethingElse foo bar');
		assert.deepEqual(result, { errorCode: ERRCODE_ARG_FILE_FAIL });
	});

	test('returns ERRCODE_ARG_FILE_FAIL and logs when parsing throws', () => {
		const sharedDir = fs.mkdtempSync(path.join(os.tmpdir(), 'decoder-error-'));
		const { messageDecoder, logs } = loadMessageDecoderWithMocks({ sharedDir });

		const result = messageDecoder('import -f $SHARED_SPACE/missing.json');

		assert.deepEqual(result, { errorCode: ERRCODE_ARG_FILE_FAIL });
		assert.equal(logs.error.length, 1);
		assert.equal(logs.error[0][0].startsWith('Failed to parse message:'), true);
	});
});
