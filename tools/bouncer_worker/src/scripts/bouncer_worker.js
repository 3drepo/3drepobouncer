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

const { exitAfter, queue, migrateUnityBundles } = require('../lib/processParams');
const { exitApplication } = require('../lib/utils');
const { connectToQueue, runNTasks } = require('../lib/queueHandler');
const { testClient } = require('../tasks/bouncerClient');
const { testImageClient } = require('../tasks/imageProcessing');
const logger = require('../lib/logger');
const { runUnityBundleMigration } = require('../migration/migrateUnityBundles');

const startBouncerWorker = async () => {
	try {
		await testClient();

		if (migrateUnityBundles !== undefined) {
			await runUnityBundleMigration(migrateUnityBundles);
			return; // Migration is a one-shot job
		}

		if (!queue || queue === 'drawing') {
			await testImageClient();
		}

		if (exitAfter > 0) {
			if (!queue) {
				throw '--queue must be specified running with --exitAfter option';
			}

			runNTasks(queue, exitAfter);
		} else {
			connectToQueue(queue);
		}
	} catch (err) {
		logger.error(`Error occured: ${err}`);
		exitApplication();
	}
};

startBouncerWorker();
