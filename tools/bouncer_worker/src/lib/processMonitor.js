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
const dataByModel = {};
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
		logger.error(`[maxmem]: ${err}`, logLabel);
	}
};

const monitor = (pid) => setInterval(() => updateMemory(pid), memoryIntervalMS);

const shouldMonitor = async () => enabled && permittedOS.includes(await currentOSPromise);
const monitorEnabled = shouldMonitor();

ProcessMonitor.startMonitor = async (startPID, processInfo) => {
	if (!(await monitorEnabled)) return;
	if (dataByModel[processInfo.model]) delete dataByModel[processInfo.model];
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
	if (!(await monitorEnabled) || !dataByPid[stopPID]) return;

	const { processInfo, maxMemory, startMemory, startTime, timer } = dataByPid[stopPID];
	clearInterval(timer);
	const report = {
		...processInfo,
		ReturnCode: returnCode,
		MaxMemory: maxMemory - startMemory,
		ProcessTime: Date.now() - startTime,
	};

	dataByModel[report.model] = report;
	logger.verbose(`Stopping monitoring for ${stopPID} MaxMemory = ${report.MaxMemory}`, logLabel);
	// Ensure there's no race condition with the last interval being processed
	await sleep(memoryIntervalMS);
	delete dataByPid[stopPID];
};

ProcessMonitor.sendReport = async (model) => {
	if (!(await monitorEnabled) || !dataByModel[model]) return;
	const report = dataByModel[model];
	logger.verbose(`Sending report for ${model}`, logLabel);
	if (elasticEnabled) {
		try {
			await Elastic.createProcessRecord(report);
		} catch (err) {
			logger.error('Failed to create elastic record', report, logLabel);
		}
	} else {
		logger.info(`${model} stats ProcessTime: ${report.ProcessTime} MaxMemory: ${report.MaxMemory}`, logLabel);
	}

	// Ensure there's no race condition with the last interval being processed
	await sleep(memoryIntervalMS);
	delete dataByModel[model];
};

ProcessMonitor.clearReport = async (model) => {
	if (!(await monitorEnabled) || !dataByModel[model]) return;
	const report = dataByPid[model];
	logger.info(`${model} stats ProcessTime: ${report.ProcessTime} MaxMemory: ${report.MaxMemory}`, logLabel);
	// Ensure there's no race condition with the last interval being processed
	await sleep(memoryIntervalMS);
	delete dataByModel[model];
};

module.exports = ProcessMonitor;
