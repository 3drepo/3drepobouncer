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


module.exports = function(dbConfig, dir, username, database, project){

	const mongodb = require('mongodb');
	const GridFSBucket = mongodb.GridFSBucket;
	const MongoClient = mongodb.MongoClient;
	const fs = require('fs');

	function importJSON(){
		let importCollectionFiles = {};
		
		fs.readdirSync(`${__dirname}/${dir}`).forEach(file => {
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


		//mongoimport all json first

		Object.keys(importCollectionFiles).forEach(collection => {

			let filename = importCollectionFiles[collection];

			promises.push(new Promise((resolve, reject) => {

				require('child_process').exec(
				`mongoimport -j 8 --host ${hostString} --username ${dbUsername} --password ${dbPassword} --authenticationDatabase admin --db ${database} --collection ${collection} --file ${dir}/${filename}`,
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
				'author': username,
				'timestamp': new Date()
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

	let db;
	const url = `mongodb://${dbConfig.username}:${dbConfig.password}@${dbConfig.dbhost}:${dbConfig.dbport}/admin`;

	return MongoClient.connect(url).then(_db => {

		db = _db.db(database);
		return importJSON();

	}).then(() => {

		return Promise.all([
			renameStash(db, `${project}.stash.json_mpc`),
			renameStash(db, `${project}.stash.src`)
		]);

	}).then(() => {

		return updateHistoryAuthorAndDate(db);

	}).then(() => {

		return updateIssueAuthorAndDate(db);

	});

}