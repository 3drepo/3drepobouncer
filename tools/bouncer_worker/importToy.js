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


module.exports = function(dbConfig, modelDir, username, database, project, skipPostProcessing){

	const mongodb = require('mongodb');
	const GridFSBucket = mongodb.GridFSBucket;
	const MongoClient = mongodb.MongoClient;
	const fs = require('fs');
	skipPostProcessing = skipPostProcessing || {};


	let db;
	const url = `mongodb://${dbConfig.username}:${dbConfig.password}@${dbConfig.dbhost}:${dbConfig.dbport}/admin`;

	return MongoClient.connect(url).then(_db => {

		db = _db.db(database);
		return importJSON();

	}).then(() => {


		return !skipPostProcessing.renameStash && Promise.all([
			renameStash(db, `${project}.stash.json_mpc`),
			renameStash(db, `${project}.stash.src`),
			renameStash(db, `${project}.stash.unity3d`)
		]);

	}).then(() => {

		return !skipPostProcessing.history && updateHistoryAuthorAndDate(db);

	}).then(() => {

		return !skipPostProcessing.issues && updateIssueAuthorAndDate(db);

	}).then(() => {

		return !skipPostProcessing.groups && renameGroups(db);

	}).then(() => {

		console.log('Finish importing toy model!');

	});


	function importJSON(){
		let importCollectionFiles = {};
		
		fs.readdirSync(`${__dirname}/${modelDir}`)
		.forEach(file => {
			// remove '.json' in string
			let collectionName = file.split('.');
			collectionName.pop();
			collectionName = collectionName.join('.');

			importCollectionFiles[`${project}.${collectionName}`] = file;
		});

		let dbUsername = dbConfig.username;
		let dbPassword = dbConfig.password;

		let promises = [];
		let hostString = `${dbConfig.dbhost}:${dbConfig.dbport}`;
		let writeConcern = dbConfig.writeConcern || {w: 1};

		//mongoimport all json first

		Object.keys(importCollectionFiles).forEach(collection => {

			let filename = importCollectionFiles[collection];

			promises.push(new Promise((resolve, reject) => {
				var cmd = `mongoimport -j 8 --host ${hostString} --username ${dbUsername} --password ${dbPassword} --authenticationDatabase admin --db ${database} --collection ${collection} --writeConcern '${JSON.stringify(writeConcern)}' --file ${__dirname}/${modelDir}/${filename}`;
				require('child_process').exec(cmd,
				{
					cwd: __dirname
				}, function (err) {
					if(err){
						reject({message: err.message.replace(new RegExp(dbPassword, 'g'), '[password masked]').replace(new RegExp(dbUsername, 'g'), '[username masked]')});
					} else {
						resolve();
					}
				});

			}));
		});

		return Promise.all(promises);
	}

	function renameStash(db, bucketName){

		let bucket = new GridFSBucket(db, { bucketName });

		return new Promise((resolve, reject) => {

			let renamePromises = [];

			bucket.find().forEach(file => {

				let newFileName = file.filename;
				newFileName = newFileName.split('/');
				newFileName[1] = database;
				newFileName[2] = project;
				newFileName = newFileName.join('/');
				file.filename = newFileName;

				renamePromises.push(

					new Promise((resolve, reject) => {

						bucket.rename(file._id, newFileName, function(err) {
							if(err){
								reject(err);
							} else {
								resolve();
							}

						});
					})

				);

				// unityAssets.json have the path baked into the file :(
				if(newFileName.endsWith('unityAssets.json')){
					renamePromises.push(renameUnityAsset(bucket, file));
				}

			}, err => {
				if(err){
					reject(err);
				} else {
					Promise.all(renamePromises)
						.then(() => resolve())
						.catch(err => reject(err));
				}
			});

		});
	}

	function updateHistoryAuthorAndDate(db){
		
		const collection = db.collection(`${project}.history`);
		const update = {
			'$set':{
				'author': username
			}
		};

		return collection.updateMany({}, update);
	}

	function updateIssueAuthorAndDate(db){

		return new Promise((resolve, reject) => {

			const promises = [];
			const collection = db.collection(`${project}.issues`);

			collection.find().forEach(issue => {

				issue.owner = username;
				issue.comments && issue.comments.forEach(comment => {
					comment.owner = username;
				});

				promises.push(collection.updateOne({ _id: issue._id }, issue));

			}, function done(err) {
				if(err){
					reject(err);
				} else {
					Promise.all(promises)
						.then(() => resolve())
						.catch(err => reject(err));
				}
			});
		});
	}

	function renameGroups(db){

		const subModelNameToOldID = {
			"Lego_House_Architecture" : "ae234e5d-3b75-4b8c-b461-7529d5bec583",
			"Lego_House_Landscape" : "39b51fe5-0fcb-4734-8e10-d8088bec9d45",
			"Lego_House_Structure" : "3749c3cc-375d-406a-9f0d-2d059e8e782f"
		}; 
		
		return new Promise((resolve, reject) => {

			const updateGroupPromises = [];
			const collection = db.collection(`${project}.groups`);


			return db.collection("settings").findOne({_id: project}).then(setting => {
				const oldIdToNewId = {};
				let subModelPromise;
				if(setting.subModels) {
					const subModelList = [];
					setting.subModels.forEach((subModel) => {
						subModelList.push(subModel.model);
					});
					console.log("!!!!!!!!! sub model list", subModelList); 
					subModelPromise = db.collection("settings").find({_id: {$in: subModelList}}).toArray();
					subModelPromise.then((arr) => {
						arr.forEach( (subModelSetting) => {
							if(subModelNameToOldID[subModelSetting.name]) {
								console.log(subModelSetting.name, subModelSetting._id, "=>", subModelSetting._id);
								oldIdToNewId[subModelNameToOldID[subModelSetting.name]] = subModelSetting._id;
							}
						});
					});
				} 
				else {
					subModelPromise = Promise.resolve();
				}

					
				console.log("!!!!!!!!!!!! ID Mapping" , oldIdToNewId);

				collection.find().forEach(group => {
					group.objects && group.objects.forEach(obj => {
						const updateObjectPromises = [];
						obj.account = database;
					
						//if model is fed model, then model id of a group should be 
						//one of the sub models instead of the id of the fed model itself
						if(oldIdToNewId[project]) {
							obj.model = oldIdToNewId[project];
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

	}

	function renameUnityAsset(bucket, file){
		new Promise((resolve, reject) => {

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
						newFileName[2] = project;
						files[i] = newFileName.join('/');
					});
				})

				unityAsset.database = database;
				unityAsset.model = project;


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

}
