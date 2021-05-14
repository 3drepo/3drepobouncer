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
		const exists = await processExists.all(Array.from(pidSet));
		for (const pid of pidSet) {
			if (!exists.get(pid)) {
				pidSet.delete(pid);
			}
		}
		if (pidSet.size > 0) {
			pids = await pidusage(Array.from(pidSet));
			for (const pid in pids) {
				if (pid in workingDict) {
					workingDict[pid].elapsedTime = pids[pid].elapsed;
					if (data > workingDict[pid].maxMemory) {
						logger.verbose(`Updating MaxMemory for ${pid} to ${data}`, logLabel);
						workingDict[pid].maxMemory = data;
					}
				}
			}
		}
	} catch (err) { logger.error(`[maxmem]: ${err}`, logLabel); }
};

// Compute statistics every interval:
const interval = async (time) => {
	if (pidSet.size > 0) {
		setTimeout(async () => {
			await maxmem();
			interval(time);
		}, time);
	}
};

const monitor = async () => {
	interval(memoryIntervalMS);
};

ProcessMonitor.startMonitor = async (startPID, processInformation) => {
	const currentOS = await currentOSPromise;
	const currentMemUsage = await getCurrentMemUsage();
	if (permittedOS.includes(currentOS)) {
		pidSet.add(startPID);
		informationDict[startPID] = processInformation;
		workingDict[startPID] = { startMemory: currentMemUsage, maxMemory: currentMemUsage };
		monitor();
		logger.verbose(`Monitoring enabled for process ${startPID} starting at ${workingDict[startPID].startMemory}`, logLabel);
	} else {
		logger.error(`[${currentOS}]: not a supported operating system for monitoring.`, logLabel);
	}
};

ProcessMonitor.stopMonitor = async (stopPID, returnCode) => {
	const currentOS = await currentOSPromise;
	if (permittedOS.includes(currentOS)) {
		// take the PID out of circulation for checking immediately.
		pidSet.delete(stopPID);

		// add the missing properties for sending.
		informationDict[stopPID].ReturnCode = returnCode;
		informationDict[stopPID].MaxMemory = workingDict[stopPID].maxMemory - workingDict[stopPID].startMemory;
		logger.verbose(`${stopPID} ${workingDict[stopPID].maxMemory} - ${workingDict[stopPID].startMemory} = ${informationDict[stopPID].MaxMemory}`, logLabel);
		informationDict[stopPID].ProcessTime = workingDict[stopPID].elapsedTime;

		if (enabled) {
			try {
				Elastic.createRecord(informationDict[stopPID]);
			} catch (err) {
				logger.error('Failed to create record', logLabel);
			}
		} else {
			logger.info(`${stopPID} stats ProcessTime: ${informationDict[stopPID].ProcessTime} MaxMemory: ${informationDict[stopPID].MaxMemory}`, logLabel);
		}

		delete informationDict[stopPID];
		delete workingDict[stopPID];
	} else {
		logger.error(`[${currentOS}]: not a supported operating system for monitoring.`, logLabel);
	}
};

module.exports = ProcessMonitor;
