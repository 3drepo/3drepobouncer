const si = require('systeminformation');
const fs = require('fs');
const logger = require('./logger');
const Elastic = require('./elastic');
const { enabled, memoryIntervalMS } = require('./config').config.processMonitoring;
const elasticEnabled = require('./config').config.elastic;
const { sleep } = require('./utils');

const ProcessMonitor = {};
const logLabel = { label: 'PROCESSMONITOR' };

const dataByPid = {};
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

const updateMemory = async (pid) => {
	try {
		if (dataByPid[pid]) {
			const data = await getCurrentMemUsage();
			dataByPid[pid].maxMemory = Math.max(dataByPid[pid].maxMemory, data);
		}
	} catch (err) {
		if (dataByPid[pid]) {
			logger.error(`[ProcessMonitor.updateMemory]: ${err}`, logLabel);
		}
	}
};

const monitor = (pid) => setInterval(() => updateMemory(pid), memoryIntervalMS);

const shouldMonitor = async () => enabled && permittedOS.includes(await currentOSPromise);

ProcessMonitor.startMonitor = async (startPID, processInfo) => {
	if (!(await shouldMonitor())) return;
	if (dataByRid[processInfo.Rid]) delete dataByRid[processInfo.Rid];
	const currentMemUsage = await getCurrentMemUsage();
	dataByPid[startPID] = {
		startMemory: currentMemUsage,
		maxMemory: currentMemUsage,
		startTime: Date.now(),
		processInfo };
	dataByPid[startPID].timer = monitor(startPID);
	logger.verbose(`Monitoring enabled for process ${startPID} starting at ${dataByPid[startPID].startMemory}`, logLabel);
};

ProcessMonitor.stopMonitor = async (stopPID, returnCode) => {
	if (!(await shouldMonitor()) || !dataByPid[stopPID]) return;
	const { processInfo, maxMemory, startMemory, startTime, timer } = dataByPid[stopPID];
	clearInterval(timer);
	const report = {
		...processInfo,
		ReturnCode: returnCode,
		MaxMemory: maxMemory - startMemory,
		ProcessTime: Date.now() - startTime,
	};

	dataByRid[report.Rid] = report;
	logger.verbose(`Stopping monitoring for ${stopPID} MaxMemory = ${report.MaxMemory}`, logLabel);
	// Ensure there's no race condition with the last interval being processed
	await sleep(memoryIntervalMS);
	delete dataByPid[stopPID];
};

ProcessMonitor.sendReport = async (ridString) => {
	if (!(await shouldMonitor()) || !dataByRid[ridString]) return;
	const report = dataByRid[ridString];
	logger.verbose(`Sending report for ${ridString}`, logLabel);
	if (elasticEnabled) {
		try {
			await Elastic.createProcessRecord(report);
		} catch (err) {
			logger.error('Failed to create elastic record', report, logLabel);
		}
	} else {
		logger.info(`${ridString} stats ProcessTime: ${report.ProcessTime} MaxMemory: ${report.MaxMemory}`, logLabel);
	}

	// Ensure there's no race condition with the last interval being processed
	await sleep(memoryIntervalMS);
	delete dataByRid[ridString];
};

ProcessMonitor.clearReport = async (ridString) => {
	if (!(await shouldMonitor()) || !dataByRid[ridString]) return;
	const report = dataByRid[ridString];
	logger.info(`${ridString} stats ProcessTime: ${report.ProcessTime} MaxMemory: ${report.MaxMemory}`, logLabel);
	// Ensure there's no race condition with the last interval being processed
	await sleep(memoryIntervalMS);
	delete dataByRid[ridString];
};

module.exports = ProcessMonitor;
