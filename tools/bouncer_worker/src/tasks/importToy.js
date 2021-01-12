/**
 *  Copyright (C) 2017 3D Repo Ltd
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

const { MongoClient, GridFSBucket } = require('mongodb');
const fs = require('fs');
const { config } = require('../lib/config');
const runCommand = require('../lib/runCommand');
const logger = require('../lib/logger');

const logLabel = { label: 'TOY' };

const accumulateCollectionFiles = (modelDir, modelId) => {
	const importCollectionFiles = {};

	fs.readdirSync(modelDir).forEach((file) => {
		// remove '.json' in string
		let collectionName = file.split('.');
		collectionName.pop();
		collectionName = collectionName.join('.');

		importCollectionFiles[`${modelId}.${collectionName}`] = file;
	});

	return importCollectionFiles;
};

const runMongoImport = async (database, collection, filePath) => {
	const params = [
		'-j', '8',
		'--host', `${config.db.dbhost}:${config.db.dbport}`,
		'--username', config.db.username,
		'--password', config.db.password,
		'--authenticationDatabase', 'admin',
		'--db', database,
		'--collection', collection,
		'--writeConcern', JSON.stringify(config.mongoimport.writeConcern),
		'--file', filePath,
	];

	try {
		await runCommand('mongoimport', params, { verbose: false });
	} catch (errCode) {
		logger.error(`Failed to run mongoimport on ${database}:${collection} with data from ${filePath}`, logLabel);
		throw errCode;
	}
};

const importJSON = async (modelDir, database, modelId) => {
	const collectionFiles = accumulateCollectionFiles(modelDir, modelId);

	const promises = [];
	Object.keys(collectionFiles).forEach((collection) => {
		const files = collectionFiles[collection];
		const filePath = `${modelDir}/${files}`;
		promises.push(runMongoImport(database, collection, filePath));
	});

	return Promise.all(promises);
};

const updateHistoryAuthorAndDate = (db, database, modelId) => {
	const collection = db.collection(`${modelId}.history`);
	const update = {
		$set: {
			author: database,
		},
	};
	return collection.updateMany({}, update);
};

const updateAuthorAndDate = async (db, database, model, ext) => {
	const collection = db.collection(`${model}.${ext}`);
	const promises = [];

	const issues = await collection.find().toArray();
	issues.forEach((issue) => {
		const updatedIssue = issue;
		updatedIssue.owner = database;
		const comments = [];
		if (updatedIssue.comments) {
			updatedIssue.comments.forEach((comment) => {
				comments.push({ ...comment, owner: database });
			});
		}
		updatedIssue.comments = comments;
		promises.push(collection.updateOne({ _id: updatedIssue._id }, updatedIssue));
	});

	await Promise.all(promises);
};

const renameUnityAssetList = (db, database, model) => {
	const collection = db.collection(`${model}.stash.unity3d`);
	const promises = [];
	const prefix = `/${database}/${model}/`;
	collection.find().forEach((asset) => {
		const entry = asset;
		entry.database = database;
		entry.model = model;
		for (let i = 0; i < entry.jsonFiles.length; ++i) {
			const oldDir = entry.jsonFiles[i];
			const dirArr = oldDir.split('/');
			entry.jsonFiles[i] = prefix + dirArr[dirArr.length - 1];
		}

		for (let i = 0; i < entry.assets.length; ++i) {
			const oldDir = entry.assets[i];
			const dirArr = oldDir.split('/');
			entry.assets[i] = prefix + dirArr[dirArr.length - 1];
		}

		promises.push(collection.updateOne({ _id: entry._id }, entry));
	});
	return Promise.all(promises);
};

const renameUnityAsset = (bucket, database, modelId, file) => new Promise((resolve, reject) => {
	const rs = bucket.openDownloadStream(file._id);
	const bufs = [];

	function finishDownload() {
		const unityAsset = JSON.parse(Buffer.concat(bufs));

		unityAsset.assets = unityAsset.assets || [];
		unityAsset.jsonFiles = unityAsset.jsonFiles || [];

		[unityAsset.assets, unityAsset.jsonFiles].forEach((arr) => {
			arr.forEach((assetFile, i, assetFiles) => {
				const newFileName = assetFile.split('/');
				newFileName[1] = database;
				newFileName[2] = modelId;
				// eslint-disable-next-line no-param-reassign
				assetFiles[i] = newFileName.join('/');
			});
		});

		unityAsset.database = database;
		unityAsset.model = modelId;

		// drop the old one
		bucket.delete(file._id, (err) => {
			if (err) {
				reject(err);
			}

			// write the updated unityasset json back to database
			const ws = bucket.openUploadStreamWithId(file._id, file.filename);

			ws.end(JSON.stringify(unityAsset), 'utf8', (error) => {
				if (error) {
					reject(error);
				} else {
					resolve();
				}
			});
		});
	}

	rs.on('data', (d) => bufs.push(d));
	rs.on('end', () => finishDownload());
	rs.on('error', (err) => reject(err));
});

const renameStash = (db, database, modelId, bucketName) => {
	const bucket = new GridFSBucket(db, { bucketName });
	const renamePromises = [];

	const files = bucket.find();
	for (let i = 0; i < files.length; ++i) {
		const file = files[i];
		let newFileName = file.filename.split('/');
		if (newFileName.length >= 3) {
			newFileName[1] = database;
			newFileName[2] = modelId;
			newFileName = newFileName.join('/');
			file.filename = newFileName;

			renamePromises.push(bucket.rename(file._id, newFileName));

			// unityAssets.json have the path baked into the file :(
			if (newFileName.endsWith('unityAssets.json')) {
				renamePromises.push(renameUnityAsset(bucket, database, modelId, file));
			}
		}
	}

	return Promise.all(renamePromises);
};

const renameGroups = async (db, database, modelId) => {
	const subModelNameToOldID = {
		Lego_House_Architecture: '1cac0310-e3cc-11ea-bc6b-69e466be9639',
		Lego_House_Landscape: '1cab8de0-e3cc-11ea-bc6b-69e466be9639',
		Lego_House_Structure: '1cac5130-e3cc-11ea-bc6b-69e466be9639',
	};

	const collection = db.collection(`${modelId}.groups`);

	const setting = db.collection('settings').findOne({ _id: modelId });

	if (!setting) {
		throw `Model ${setting} not found`;
	}

	const oldIdToNewId = {};

	if (setting.subModels) {
		const subModelList = [];
		setting.subModels.forEach((subModel) => {
			subModelList.push(subModel.model);
		});
		const submodels = await db.collection('settings').find({ _id: { $in: subModelList } }).toArray();
		submodels.forEach((subModelSetting) => {
			if (subModelNameToOldID[subModelSetting.name]) {
				oldIdToNewId[subModelNameToOldID[subModelSetting.name]] = subModelSetting._id;
			}
		});
	}

	const groups = await collection.find().toArray();
	const updateObjectPromises = [];

	groups.forEach((group) => {
		const grp = group;
		grp.author = database;
		if (group.objects) {
			group.objects.forEach((object) => {
				const obj = object;
				obj.account = database;

				// if model is fed model, then model id of a group should be
				// one of the sub models instead of the id of the fed model itself
				obj.model = oldIdToNewId[obj.model] || modelId;
			});
		}
		updateObjectPromises.push(collection.updateOne({ _id: group._id }, group));
	});

	await Promise.all(updateObjectPromises);
};

const ToyImporter = {};

ToyImporter.importToyModel = async (toyModelID, database, modelId) => {
	const modelDir = `${config.toyModelDir}/${toyModelID}`;
	await importJSON(modelDir, database, modelId);

	const url = `mongodb://${config.db.username}:${config.db.password}@${config.db.dbhost}:${config.db.dbport}/admin`;
	const dbConn = await MongoClient.connect(url);
	const db = dbConn.db(database);

	const promises = [];

	promises.push(renameStash(db, database, modelId, `${modelId}.stash.json_mpc`));
	promises.push(renameStash(db, database, modelId, `${modelId}.stash.src`));
	promises.push(renameStash(db, database, modelId, `${modelId}.stash.unity3d`));
	promises.push(renameUnityAssetList(db, database, modelId));

	promises.push(updateHistoryAuthorAndDate(db, database, modelId));
	promises.push(updateAuthorAndDate(db, database, modelId, 'issues'));
	promises.push(updateAuthorAndDate(db, database, modelId, 'risks'));
	promises.push(renameGroups(db, database, modelId));

	await Promise.all(promises);
	logger.info(`${toyModelID} imported to ${modelId}`, logLabel);
};

ToyImporter.validateToyImporterSettings = () => {
	if (!config.toyModelDir) {
		logger.error('toyModelDir not specified', logLabel);
		return false;
	}

	return true;
};

module.exports = ToyImporter;
