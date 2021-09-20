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
const { elastic } = require('./config').config;

const Elastic = {};
const bouncerIndexPrefix = 'io-bouncer';

const processingRecordMapping = {
	DateTime: { type: 'date' },
	Owner: { type: 'keyword' },
	Model: { type: 'keyword' },
	Database: { type: 'keyword' },
	Queue: { type: 'keyword' },
	FileType: { type: 'keyword'	},
	FileSize: { type: 'double' },
	MaxMemory: { type: 'double' },
	ProcessTime: { type: 'double' },
	ReturnCode: { type: 'double' },
};

const indicesMappings = [
	{
		index: bouncerIndexPrefix,
		mapping: processingRecordMapping,
	},
];

const logLabel = { label: 'ELASTIC' };

const establishIndices = async (client) => Promise.all(
	indicesMappings.map(async ({ index, mapping }) => {
		const { body } = await client.indices.exists({ index });
		if (!body) {
			await client.indices.create({ index });
			logger.info(`Created index ${index}`);
			if (mapping) {
				await client.indices.putMapping({
					index,
					body: { properties: mapping },
				});
			}
			logger.info(`Created mapping ${index}`);
		}
	}),
);

const createElasticClient = async () => {
	if (!elastic) return undefined;
	const internalElastic = new Client(elastic);
	try {
		await internalElastic.cluster.health();
		logger.info(`Succesfully connected to ${elastic.cloud.id}`, logLabel);
	} catch (err) {
		logger.error('Health check failed on elastic connection, please check settings.', logLabel);
		Utils.exitApplication();
	}

	await establishIndices(internalElastic);
	return internalElastic;
};

const elasticClientPromise = createElasticClient();
const createElasticRecord = async (index, body, id) => {
	try {
		const elasticClient = await elasticClientPromise;
		if (!elasticClient) {
			return;
		}

		await elasticClient.index({
			index,
			id,
			refresh: true,
			body,
		});
		logger.verbose(`created doc ${index} ${Object.values(body).toString()}`, logLabel);
	} catch (error) {
		logger.error(`createElasticRecord ${error} ${index}`, logLabel);
	}
};

Elastic.createProcessRecord = async (elasticBody) => {
	if (elasticBody) {
		await createElasticRecord(
			bouncerIndexPrefix,
			elasticBody,
			Utils.hashCode(Object.values(elasticBody).toString()),
		);
	}
};

module.exports = Elastic;
