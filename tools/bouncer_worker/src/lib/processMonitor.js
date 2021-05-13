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
const permittedOS = ['linux', 'windows'];
// const getCurrentMemUsage = () =>  Number(fs.readFileSync('/sys/fs/cgroup/memory/memory.usage_in_bytes'));

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
			// console.log (stats);
			for (const pid in pids) {
				// console.log(stats[stat].pid)
				// console.log(workingDict[stat].maxMemory)
				// console.log(data)
				if (pid in workingDict) {
					workingDict[pid].elapsedTime = pids[pid].elapsed;
					if (data > workingDict[pid].maxMemory) {
						// let current = workingDict[stat].maxMemory
						// logger.verbose(`${stat}, ${data}, ${current}`, logLabel)
						workingDict[pid].maxMemory = data;
					}
				} else {
					// console..log(workingDict);
				}
			}
			// if (data > maxMemory) maxMemory = data;
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
	logger.verbose(`[${currentOS}]: booting!`, logLabel);
	if (currentOS in permittedOS) {
		pidSet.add(inputPID);
		informationDict[inputPID] = processInformation;
		logger.verbose(informationDict[inputPID].toString(), logLabel);
		workingDict[inputPID] = { startMemory: 0, maxMemory: 0 };
		workingDict[inputPID].startMemory = await getCurrentMemUsage();
		monitor();
		logger.verbose(`[${inputPID}]: a startMonitor event occurred!`, logLabel);
	} else {
		logger.verbose(`[${currentOS}]: not linux?!`, logLabel);
	}
};

ProcessMonitor.stopMonitor = async (inputPID, returnCode) => {
	logger.verbose(`[${inputPID}]: a stopMonitor event occurred! returnCode: ${returnCode}`, logLabel);

	const currentOS = await currentOSPromise;
	if (currentOS in permittedOS) {
		// take the PID out of circulation for checking immediately.
		pidSet.delete(inputPID);

		// add the missing properties for sending.
		informationDict[inputPID].ReturnCode = returnCode;
		informationDict[inputPID].MaxMemory = workingDict[inputPID].maxMemory - workingDict[inputPID].startMemory;
		informationDict[inputPID].ProcessTime = workingDict[inputPID].elapsedTime;

		workingDict[inputPID] = { startMemory: 0, maxMemory: 0 };

		// console.log(informationDict[inputPID]);

		if (enabled) {
			try {
				Elastic.createRecord(informationDict[inputPID]);
			} catch (err) {
				logger.error('Failed to create record', logLabel);
			}
		}

		delete informationDict[inputPID];
		delete workingDict[inputPID];
	} else {
		logger.verbose(`[${currentOS}]: not linux?!`, logLabel);
	}
};

module.exports = ProcessMonitor;
