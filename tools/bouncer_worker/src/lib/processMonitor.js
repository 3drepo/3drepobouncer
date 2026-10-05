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

const { enabled, memoryIntervalMS } = require('./config').config.processMonitoring;
const elasticEnabled = require('./config').config.elastic;
const Elastic = require('./elastic');
const fs = require('fs');
const logger = require('./logger');
const si = require('systeminformation');
const { sleep } = require('./utils');

const ProcessMonitor = {};
const logLabel = { label: 'PROCESSMONITOR' };

const dataByRid = {};
const permittedOS = ['linux', 'win32'];

const getCurrentOperatingSystem = async () => {
	try {
		return (await si.osInfo()).platform;
	} catch (e) {
		logger.error('Failed to get operating system information record', logLabel);
	}

	return undefined;
};
const currentOSPromise = getCurrentOperatingSystem();

const getDockerEnvironment = async () => {
	const currentOS = await currentOSPromise;
	return currentOS === 'linux' && fs.existsSync('/.dockerenv');
};
const isDockerEnvironment = getDockerEnvironment();

const getCurrentMemUsage = async () => {
	let result = 0;
	try {
		if (await isDockerEnvironment) {
			// required to get more accurate information in docker
			result = Number(fs.readFileSync('/sys/fs/cgroup/memory/memory.usage_in_bytes'));
		} else {
			result = (await si.mem()).used;
		}
	} catch (e) {
		logger.error('Failed to get memory information record', logLabel);
	}
	return result;
};

const updateMemory = async (rid) => {
	try {
		if (dataByRid[rid]) {
			const data = await getCurrentMemUsage();
			dataByRid[rid].maxMemory = Math.max(dataByRid[rid].maxMemory, data);
		}
	} catch (err) {
		if (dataByRid[rid]) {
			logger.error(`[ProcessMonitor.updateMemory]: ${err}`, logLabel);
		}
	}
};

const monitor = (rid) => setInterval(() => updateMemory(rid), memoryIntervalMS);

const shouldMonitor = async () => enabled && permittedOS.includes(await currentOSPromise);

ProcessMonitor.startMonitor = async (processInfo) => {
	if (!(await shouldMonitor())) return;
	const currentMemUsage = await getCurrentMemUsage();
	dataByRid[processInfo.Rid] = {
		startMemory: currentMemUsage,
		maxMemory: currentMemUsage,
		startTime: Date.now(),
		processInfo,
		prevRecord: dataByRid[processInfo.Rid],
	};
	dataByRid[processInfo.Rid].timer = monitor(processInfo.Rid);
	logger.verbose(`Monitoring enabled for revision ${processInfo.Rid} starting at ${dataByRid[processInfo.Rid].startMemory}`, logLabel);
};

ProcessMonitor.stopMonitor = async (rid, returnCode) => {
	if (!(await shouldMonitor()) || !dataByRid[rid]) return;
	const { processInfo, maxMemory, startMemory, startTime, timer, prevRecord } = dataByRid[rid];
	clearInterval(timer);
	const report = {
		...processInfo,
		ReturnCode: returnCode,
		MaxMemory: Math.max(maxMemory - startMemory, prevRecord?.MaxMemory ?? 0),
		ProcessTime: (prevRecord?.ProcessTime ?? 0) + (Date.now() - startTime),
	};

	dataByRid[rid] = report;
	logger.verbose(`Stopping monitoring for ${rid} MaxMemory = ${report.MaxMemory}`, logLabel);
	// Ensure there's no race condition with the last interval being processed
	await sleep(memoryIntervalMS);
};

ProcessMonitor.sendReport = async (rid) => {
	if (!(await shouldMonitor()) || !dataByRid[rid]) return;
	const report = dataByRid[rid];
	logger.verbose(`Sending report for ${rid}`, logLabel);
	if (elasticEnabled) {
		try {
			await Elastic.createProcessRecord(report);
		} catch (err) {
			logger.error('Failed to create elastic record', report, logLabel);
		}
	} else {
		logger.info(`${rid} stats ProcessTime: ${report.ProcessTime} MaxMemory: ${report.MaxMemory}`, logLabel);
	}

	// Ensure there's no race condition with the last interval being processed
	await sleep(memoryIntervalMS);
	delete dataByRid[rid];
};

ProcessMonitor.clearReport = async (rid) => {
	if (!(await shouldMonitor()) || !dataByRid[rid]) return;
	const report = dataByRid[rid];
	logger.info(`${rid} stats ProcessTime: ${report.ProcessTime} MaxMemory: ${report.MaxMemory}`, logLabel);
	// Ensure there's no race condition with the last interval being processed
	await sleep(memoryIntervalMS);
	delete dataByRid[rid];
};

module.exports = ProcessMonitor;
