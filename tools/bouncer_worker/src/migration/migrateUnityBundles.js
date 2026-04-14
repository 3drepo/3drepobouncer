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

// Ideally the driver would be able to manage any load itself without
// stability issues, but experience tells us it can't, so we use
// sequential loops in many places to slow it down...
/* eslint-disable no-await-in-loop */

const { MongoClient } = require('mongodb');
const UUIDParse = require('uuid-parse');
const logger = require('../lib/logger');
const { config } = require('../lib/config');
const BouncerHandler = require('../tasks/bouncerClient');

function UUIDToString(uuid) {
	return UUIDParse.unparse(uuid.buffer);
}

async function findUnityBundleRevisions(teamspace) {
	let { connectionString } = config.db;
	if (!connectionString) {
		connectionString = `mongodb://${config.db.username}:${config.db.password}@${config.db.dbhost}:${config.db.dbport}`;
	} else {
		connectionString = connectionString.replace('://', `://${config.db.username}:${config.db.password}@`);
	}
	const client = new MongoClient(connectionString);

	const { databases } = await client.db().admin().listDatabases({ nameOnly: 1 });

	const revisions = [];

	for (const { name } of databases) {
		if (teamspace && teamspace !== '' && name !== teamspace) {
			// eslint-disable-next-line no-continue
			continue;
		}

		logger.info(`- Checking teamspace ${name}...`);

		const db = client.db(name);
		const collections = await db.listCollections(
			{
				name: { $regex: /.unity3d$/ },
			},
			{
				nameOnly: true,
			},
		).toArray();

		let numRevisionsInTeamspace = 0;
		for (const collection of collections) {
			const container = collection.name.replace('.stash.unity3d', '');

			// Get the project for error reporting
			const project = await db.collection('projects').findOne({
				models: { $elemMatch: { $eq: container } },
			});
			if (!project) {
				logger.info(`\t- Orphaned container ${name} ${container}. Ignoring...`);
				// eslint-disable-next-line no-continue
				continue;
			}

			const history = db.collection(`${container}.history`);
			const allRevisions = await history.find({
				$or: [
					{ incomplete: { $exists: false } },
					{ incomplete: false },
				],
			},
			{
				_id: 1,
			}).toArray();

			const repoBundles = db.collection(`${container}.stash.repobundles`);
			for (const { _id } of allRevisions) {
				const numRevisionRepoBundles = await repoBundles.countDocuments({ _id });
				if (numRevisionRepoBundles === 0) {
					// This revision does not have any repobundles, and must be
					// upgraded.
					revisions.push({
						teamspace: name,
						project: UUIDToString(project._id),
						container,
						revId: UUIDToString(_id),
					});
					numRevisionsInTeamspace++;
				}
			}
		}

		logger.info(`\t- Found ${numRevisionsInTeamspace} revisions to migrate...`);
	}

	await client.close();

	return revisions;
}

async function migrateRevision(revision) {
	const logDir = `${config.logging.taskLogDir}/${revision.revId.toString()}/`;
	await BouncerHandler.generateStash(
		logDir,
		revision.teamspace,
		revision.container,
		'repo',
		revision.revId,
	);
}

async function runUnityBundleMigration(teamspace) {
	logger.info('Beginning Unity Bundle migration...');
	const revisions = await findUnityBundleRevisions(teamspace);

	logger.info(`Found ${revisions.length} revisions to migrate.`);
	const n = revisions.length;
	let i = 0;
	for (const revision of revisions) {
		logger.info(`- regenerating ${revision.teamspace} ${revision.project} ${revision.container} ${revision.revId} (${++i}/${n})`);
		await migrateRevision(revision);
	}

	logger.info(`Migrated ${i} revisions.`);
}

module.exports = {
	runUnityBundleMigration,
};
