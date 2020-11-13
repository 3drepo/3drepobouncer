'use strict';

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

const { MongoClient, GridFSBucket} = require("mongodb");
const fs = require("fs");
const config = require("../lib/config");
const { run = runCommand } = require("../lib/runCommand");

const mongoConnectionString = `mongodb://${config.db.username}:${config.db.password}@${config.db.dbhost}:${config.db.dbport}/admin`;

const accumulateCollectionFiles = (modelDir, modelId) => {
	const importCollectionFiles = {};

	fs.readdirSync(`${__dirname}/${modelDir}`).forEach(file => {
		// remove '.json' in string
		let collectionName = file.split('.');
		collectionName.pop();
		collectionName = collectionName.join('.');

		importCollectionFiles[`${modelId}.${collectionName}`] = file;
	});

	return importCollectionFiles;
}

const runMongoImport = async (database, collection, filePath) => {
	const params = [
		"-j", "8",
		"--host", `${config.db.dbhost}:${config.db.dbport}`,
		"--username", config.db.username,
		"--password", config.db.password,
		"--authenticationDatabase", "admin",
		"--db", database,
		"--collection", collection,
		"--writeConcern", JSON.stringify(config.mongoimport.writeConcern),
		"--file", filePath
	];

	try {
		await runCommand("mongoimport", params);
	} catch (errCode) {
		logger.error(`Failed to run mongoimport on ${database}:${collection} with data from ${filePath}`);
		throw errCode;
	}
}

const importJSON = async (modelDir, database, modelId) {
	const collectionFiles = accumulateCollectionFiles(modelDir, modelId);

	let promises = [];
	Object.keys(importCollectionFiles).forEach(collection => {
		const filePath = ${__dirname}/${modelDir}/${importCollectionFiles[collection]};
		promises.push(runMongoImport(database, collection, filePath));
	});

	return await Promise.all(promises);
}

const updateHistoryAuthorAndDate = (db, modelId) => {

	const collection = db.collection(`${modelId}.history`);
	const update = {
		'$set':{
			'author': username
		}
	};
	return collection.updateMany({}, update);
}

const renameStash = (db, database, modelId, bucketName) => {

	const bucket = new GridFSBucket(db, { bucketName });
	const renamePromises = [];

	bucket.find().forEach(file => {
		let newFileName = file.filename.split('/');
		if(newFileName.length >= 3) {
			newFileName[1] = database;
			newFileName[2] = modelId;
			newFileName = newFileName.join('/');
			file.filename = newFileName;

			renamePromises.push(bucket.rename(file._id, newFileName);)

			// unityAssets.json have the path baked into the file :(
			if(newFileName.endsWith('unityAssets.json')){
				renamePromises.push(renameUnityAsset(bucket, database, modelId, file));
			}
		}
	});

	return Promise.all(renamePromises);
}


const renameUnityAssetList = (db, database, model) => {

	const collection = db.collection(`${model}.stash.unity3d`);
	const promises = [];
	const prefix = `/${database}/${model}/`;
	collection.find().forEach(entry => {
		entry.database = database;
		entry.model = model;
		for(let i = 0; i < entry.jsonFiles.length; ++i) {
			const oldDir = entry.jsonFiles[i];
			const dirArr = oldDir.split("/");
			entry.jsonFiles[i] = prefix + dirArr[dirArr.length - 1];
		}

		for(let i = 0; i < entry.assets.length; ++i) {
			const oldDir = entry.assets[i];
			const dirArr = oldDir.split("/");
			entry.assets[i] = prefix + dirArr[dirArr.length - 1];
		}

		promises.push(collection.updateOne({_id: entry._id}, entry));
	});
	return Promise.all(promises);
}

const renameUnityAsset = (bucket, database, modelId, file) => {
	return new Promise((resolve, reject) => {
		const rs = bucket.openDownloadStream(file._id);
		const bufs = [];

		rs.on('data', d => bufs.push(d));
		rs.on('end', () => _finishDownload());
		rs.on('error', err => reject(err));

		function _finishDownload(){
			const unityAsset = JSON.parse(Buffer.concat(bufs));

			unityAsset.assets = unityAsset.assets || [];
			unityAsset.jsonFiles = unityAsset.jsonFiles || [];

			[unityAsset.assets, unityAsset.jsonFiles].forEach(arr => {
				arr.forEach(( file, i, files) => {
					let newFileName = file.split('/');
					newFileName[1] = database;
					newFileName[2] = modelId;
					files[i] = newFileName.join('/');
				});
			})

			unityAsset.database = database;
			unityAsset.model = modelId;


			//drop the old one
			bucket.delete(file._id, function(err){

				if(err){
					return reject(err);
				}

				//write the updated unityasset json back to database
				const ws = bucket.openUploadStreamWithId(file._id, file.filename);

				ws.end(JSON.stringify(unityAsset), 'utf8', function(err){
					if(err){
						reject(err)
					} else {
						resolve();
					}
				});
			});


		}
	});
}



const importToyModel = async (modelDir, database, modelId, skipProcessing = {}) => {
	await importJSON(modelDir, database, modelId);

	const dbConn = await MongoClient.connect(url);
	const db = dbConn.db(database);

	const promises = [];

	if(!skipPostProcessing.renameStash) {
		promises.push(renameStash(db, databse, modelId, `${modelId}.stash.json_mpc`));
		promises.push(renameStash(db, database, modelId, `${modelId}.stash.src`));
		promises.push(renameStash(db, database, modelId, `${modelId}.stash.unity3d`));
		promises.push(renameUnityAssetList(db, database, modelId));
	}

	if(!skipPostProcessing.history)
		promises.push(updateHistoryAuthorAndDate(db, modelId));
	if(!skipPostProcessing.issues)
		promises.push(updateAuthorAndDate(db, modelId, "issues"));
	if(!skipPostProcessing.risks)
		promises.push(updateAuthorAndDate(db, modelId, "risks"));
	if(!skipPostProcessing.history)
		promises.push(renameGroups(db));

	await Promise.all(promises);
	logger.log("Toy modelId imported");

}


module.exports = function(dbConfig, modelDir, username, database, project, skipPostProcessing = {}){



	function renameGroups(db){

		const subModelNameToOldID = {
			"Lego_House_Architecture" : "1cac0310-e3cc-11ea-bc6b-69e466be9639",
			"Lego_House_Landscape" : "1cab8de0-e3cc-11ea-bc6b-69e466be9639",
			"Lego_House_Structure" : "1cac5130-e3cc-11ea-bc6b-69e466be9639"
		};

		return new Promise((resolve, reject) => {

			const updateGroupPromises = [];
			const collection = db.collection(`${project}.groups`);


			return db.collection("settings").findOne({_id: project}).then(setting => {
				if(!setting) {
					return reject("Model "+ setting +" not found");
				}
				const oldIdToNewId = {};
				let subModelPromise;
				if(setting.subModels) {
					const subModelList = [];
					setting.subModels.forEach((subModel) => {
						subModelList.push(subModel.model);
					});
					subModelPromise = db.collection("settings").find({_id: {$in: subModelList}}).toArray();
					subModelPromise.then((arr) => {
						arr.forEach( (subModelSetting) => {
							if(subModelNameToOldID[subModelSetting.name]) {
								oldIdToNewId[subModelNameToOldID[subModelSetting.name]] = subModelSetting._id;
							}
						});
					});
				}
				else {
					subModelPromise = Promise.resolve();
				}

				return subModelPromise.then(() => {
					return collection.find().forEach(group => {
						group.objects && group.objects.forEach(obj => {
							const updateObjectPromises = [];
							obj.account = database;

							//if model is fed model, then model id of a group should be
							//one of the sub models instead of the id of the fed model itself
							if(oldIdToNewId[obj.model]) {
								obj.model = oldIdToNewId[obj.model];
							}
							else {
								obj.model = project;
							}
						});

						return collection.updateOne({ _id: group._id }, group);


					}, function done(err) {
						if(err){
							reject(err);
						} else {
							Promise.all(updateGroupPromises)
								.then(() => resolve())
								.catch(err => reject(err));
						}
					});

				});



			});

		});

	}


}
