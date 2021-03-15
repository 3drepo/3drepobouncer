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

Elastic.logLabel = { label: 'Elastic' };

Elastic.createElasticClient = () => {
	const ELASTIC_CLOUD_AUTH = elastic.cloudAuth.split(':');
	const config = {
		cloud: {
			id: elastic.cloudId.trim(),
		},
		auth: {
			username: ELASTIC_CLOUD_AUTH[0],
			password: ELASTIC_CLOUD_AUTH[1],
		},
		reload_connections: true,
		maxRetries: 5,
		request_timeout: 60,
	};
	const elasticClient = new Client(config);
	elasticClient.cluster.health({}, (err, resp) => {
		logger.verbose(`[ELASTIC]: ${resp}`, Elastic.loglabel);
	});
	return elasticClient;
};

Elastic.createElasticRecord = async (elasticClient, index, elasticBody, id, mapping) => {
	try {
		// eslint-disable-next-line no-param-reassign
		id = id || Utils.hashCode(Object.values(elasticBody || {}).toString());
		const indexName = index.toLowerCase(); // requirement of elastic that indexs be lowercase
		const configured = await elasticClient.indices.exists({ index: indexName });
		if (!configured.body) {
			await elasticClient.indices.create({
				index: indexName,
			});
			logger.verbose(`[ELASTIC]: Created index ${indexName}`, Elastic.loglabel);
			if (mapping) {
				await elasticClient.indices.putMapping({
					index,
					body: { properties: mapping },
				});
			}
			logger.verbose(`[ELASTIC]: Created mapping ${indexName}`, Elastic.loglabel);
		}

		if (elasticBody) {
			await elasticClient.index({
				index: indexName,
				id,
				refresh: true,
				body: elasticBody,
			});
			logger.verbose(`[ELASTIC]: created doc ${indexName} ${Object.values(elasticBody).toString()}`, Elastic.loglabel);
		}
	} catch (error) {
		logger.verbose(`[ELASTIC]: ERROR:${index}`, elasticBody, error, Elastic.loglabel);
		throw (error.body.error);
	}
};

Elastic.createElasticIndex = async (elasticClient, index, mapping) => {
	try {
		await Elastic.createElasticRecord(elasticClient, index, undefined, undefined, mapping);
	} catch (error) {
		throw (error.body.error);
	}
};

Elastic.createBouncerRecord = async (elasticBody) => {
	if (elasticBody) {
		const elasticClient = Elastic.createElasticClient();

		const activityMapping = {
			DateTime: { type: 'date' },
			Teamspace: { type: 'keyword' },
			Model: { type: 'text' },
			Database: { type: 'text' },
			FileType: { type: 'keyword' },
			FileSize: { type: 'double' },
			MaxMemory: { type: 'double' },
			ProcessTime: { type: 'double' },
			Process: { type: 'text' },
		};
		// eslint-disable-next-line
        await Elastic.createElasticRecord(elasticClient, Utils.bouncerIndexPrefix, elasticBody, undefined, activityMapping);
	}
};

Elastic.createMissingIndicies = async (elasticClient) => {
	// initialise indicies if missing

	const activityMapping = {
		Teamspace: { type: 'keyword' },
		licenseType: { type: 'text' },
		Year: { type: 'text' },
		Month: { type: 'text' },
		DateTime: { type: 'date' },
		Issues: { type: 'double' },
		'Model Revisions': { type: 'double' },
	};
	await Elastic.createElasticIndex(elasticClient, `${Utils.teamspaceIndexPrefix}-activity`, activityMapping);

	const quotaMapping = {
		Teamspace: { type: 'keyword' },
		Type: { type: 'text' },
		'User Count': { type: 'double' },
		'Max Users': { type: 'double' },
		'Max Data(GB)': { type: 'double' },
		'Expiry Date': { type: 'date' },
		Expired: { type: 'boolean' },
	};
	await Elastic.createElasticIndex(elasticClient, `${Utils.teamspaceIndexPrefix}-quota`, quotaMapping);

	const usersMapping = {
		Teamspace: { type: 'keyword' },
		Email: { type: 'text' },
		'First Name': { type: 'text' },
		'Last Name': { type: 'text' },
		Country: { type: 'text' },
		Company: { type: 'text' },
		'Date Created': { type: 'text' },
		DateTime: { type: 'date' },
		'Mail Optout': { type: 'text' },
		Verified: { type: 'boolean' },
	};
	await Elastic.createElasticIndex(elasticClient, `${Utils.teamspaceIndexPrefix}-users`, usersMapping);

	const loginMapping = {
		Teamspace: { type: 'keyword' },
		'Last Login': { type: 'text' },
		DateTime: { type: 'date' },
	};
	await Elastic.createElasticIndex(elasticClient, `${Utils.teamspaceIndexPrefix}-login`, loginMapping);

	const statsMapping = {
		Month: { type: 'text' },
		Year: { type: 'text' },
		Count: { type: 'double' },
		Total: { type: 'double' },
		DateTime: { type: 'date' },
	};
	await Elastic.createElasticIndex(elasticClient, Utils.statsIndexPrefix, statsMapping);
};

module.exports = Elastic;
