const pidusage = require('pidusage');
const processExists = require('process-exists');
const si = require('systeminformation');
// const fs = require('fs');
const logger = require('./logger');
const Elastic = require('./elastic');
const { enabled, memoryIntervalMS } = require('./config').config.elastic;

const ProcessMonitor = {};
const logLabel = { label: 'PROCESSMONITOR' };

const informationDict = {};
const workingDict = {};
const pidSet = new Set();
const permittedOS = ['linux', 'win32'];

const getCurrentMemUsage = async () => {
	let data;
	try {
		data = await si.mem();
	} catch (e) {
		logger.error('Failed to get memory information record', logLabel);
	}
	return data.used;
};

const getCurrentOperatingSystem = async () => {
	let data;
	try {
		data = await si.osInfo();
	} catch (e) {
		logger.error('Failed to get operating system information record', logLabel);
	}
	return data.platform;
};

const currentOSPromise = getCurrentOperatingSystem();

const maxmem = async () => {
	let data;
	let pids;

	try {
		data = await getCurrentMemUsage();
		if (pidSet.size > 0) {
			pids = await pidusage(Array.from(pidSet));
			for (const pid in pids) {

				if (pid in workingDict) {
					workingDict[pid].elapsedTime = pids[pid].elapsed;
					if (data > workingDict[pid].maxMemory) {

						workingDict[pid].maxMemory = data;
					}
				} 
			}
		}
	} catch (err) { logger.error(`[maxmem]: ${err}`, logLabel); }
};

// Compute statistics every interval:
const interval = async (time) => {
	const exists = await processExists.all(Array.from(pidSet));
	for (const pid of pidSet) {
		if (!exists.get(pid)) {
			pidSet.delete(pid);
		}
	}

	if (pidSet.size > 0) {
		setTimeout(async () => {
			await maxmem(exists);
			interval(time);
		}, time);
	}
};

const monitor = async () => {
	interval(memoryIntervalMS);
};

ProcessMonitor.startMonitor = async (inputPID, processInformation) => {
	const currentOS = await currentOSPromise;
	if (permittedOS.includes (currentOS)) {
		pidSet.add(inputPID);
		informationDict[inputPID] = processInformation;
		workingDict[inputPID] = { startMemory: 0, maxMemory: 0 };
		workingDict[inputPID].startMemory = await getCurrentMemUsage();
		monitor();
		logger.verbose(`Monitoring enabled for process ${inputPID}`, logLabel);
	} else {
		logger.error(`[${currentOS}]: not a supported operating system for monitoring.`, logLabel);
	}
};

ProcessMonitor.stopMonitor = async (inputPID, returnCode) => {
	const currentOS = await currentOSPromise;
	if (permittedOS.includes (currentOS)) {
		
		// take the PID out of circulation for checking immediately.
		pidSet.delete(inputPID);

		// add the missing properties for sending.
		informationDict[inputPID].ReturnCode = returnCode;
		informationDict[inputPID].MaxMemory = workingDict[inputPID].maxMemory - workingDict[inputPID].startMemory;
		informationDict[inputPID].ProcessTime = workingDict[inputPID].elapsedTime;

		workingDict[inputPID] = { startMemory: 0, maxMemory: 0 };

		if (enabled) {
			try {
				Elastic.createRecord(informationDict[inputPID]);
			} catch (err) {
				logger.error('Failed to create record', logLabel);
			}
		} else {
			logger.info(`ProcessTime: ${informationDict[inputPID].ProcessTime} MaxMemory: ${informationDict[inputPID].MaxMemory}`,logLabel)
		}

		delete informationDict[inputPID];
		delete workingDict[inputPID];

	} else {
		logger.error(`[${currentOS}]: not a supported operating system for monitoring.`, logLabel);
	}
};

module.exports = ProcessMonitor;
