/**
*  Copyright (C) 2020 3D Repo Ltd
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
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

const { Client } = require('@elastic/elasticsearch');
const logger = require('./logger');
const Utils = require('./utils');
const { cloudAuth, cloudId } = require('./config').config.elastic;

const Elastic = {};
const bouncerIndexPrefix = 'io-bouncer';

const processingRecordMapping = {
	DateTime: { type: 'date' },
	Teamspace: { type: 'keyword' },
	Model: { type: 'text' },
	Queue: { type: 'text' },
	Database: { type: 'text' },
	FileType: { type: 'keyword' },
	FileSize: { type: 'double' },
	MaxMemory: { type: 'double' },
	ProcessTime: { type: 'double' },
	ReturnCode: { type: 'double' },
};

const logLabel = { label: 'Elastic' };

const createElasticClient = async () => {
	const ELASTIC_CLOUD_AUTH = cloudAuth.split(':');
	const config = {
		cloud: {
			id: cloudId.trim(),
		},
		auth: {
			username: ELASTIC_CLOUD_AUTH[0],
			password: ELASTIC_CLOUD_AUTH[1],
		},
		reload_connections: true,
		maxRetries: 5,
		request_timeout: 60,
	};
	const internalElastic = new Client(config);
	try { 
		await internalElastic.cluster.health();
		logger.verbose("Succesfully connected to " + cloudId.trim(), loglabel)
	}
	catch (err) { 
		logger.error("Health check failed on elastic connection, please check settings.", loglabel)
		Utils.exitApplication()
	}
	return internalElastic;
};

const createElasticRecord = async (index, elasticBody, id, mapping) => {
	try {
		// eslint-disable-next-line no-param-reassign
		id = id || Utils.hashCode(Object.values(elasticBody || {}).toString());
		const indexName = index.toLowerCase(); // requirement of elastic that indexs be lowercase
		const { body } = await elasticClient.indices.exists({ index: indexName });
		if (!body) {
			await elasticClient.indices.create({
				index: indexName,
			});
			logger.verbose(`[ELASTIC]: Created index ${indexName}`, loglabel);
			if (mapping) {
				await elasticClient.indices.putMapping({
					index,
					body: { properties: mapping },
				});
			}
			logger.verbose(`[ELASTIC]: Created mapping ${indexName}`, loglabel);
		}

		if (elasticBody) {
			await elasticClient.index({
				index: indexName,
				id,
				refresh: true,
				body: elasticBody,
			});
			logger.verbose(`[ELASTIC]: created doc ${indexName} ${Object.values(elasticBody).toString()}`, loglabel);
		}
	} catch (error) {
		logger.verbose(`[ELASTIC]: ERROR:${index}`, elasticBody, error, loglabel);
		throw (error.body.error);
	}
};

const createElasticIndex = async (index, mapping) => {
	try {
		await Elastic.createElasticRecord( index, undefined, undefined, mapping);
	} catch (error) {
		throw (error.body.error);
	}
};

const createMissingIndicies = async () => {
	// initialise indicies if missing
	await createElasticIndex( bouncerIndexPrefix, processingRecordMapping);
};

Elastic.createRecord = async (elasticBody) => {
	if (elasticBody) {
        await createElasticRecord(bouncerIndexPrefix, elasticBody, undefined, processingRecordMapping );
	}
};

const elasticClient = createElasticClient()

module.exports = Elastic;
