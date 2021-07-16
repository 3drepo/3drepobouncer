const pidusage = require('pidusage');
const processExists = require('process-exists');
const si = require('systeminformation');
const fs = require('fs');
const logger = require('./logger');
const Elastic = require('./elastic');
const { enabled, memoryIntervalMS } = require('./config').config.processMonitoring;
const elasticEnabled = require('./config').config.elastic;

const ProcessMonitor = {};
const logLabel = { label: 'PROCESSMONITOR' };

const informationDict = {};
const workingDict = {};
const pidSet = new Set();
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

const maxmem = async () => {
	try {
		const data = await getCurrentMemUsage();
		const exists = await processExists.all(Array.from(pidSet));
		for (const pid of pidSet) {
			if (!exists.get(pid)) {
				pidSet.delete(pid);
			}
		}
		if (pidSet.size > 0) {
			const pids = await pidusage(Array.from(pidSet));
			for (const pid in pids) {
				if (pid in workingDict) {
					workingDict[pid].elapsedTime = pids[pid].elapsed;
					if (data > workingDict[pid].maxMemory) {
						logger.debug(`Updating MaxMemory for ${pid} to ${data}`, logLabel);
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

const shouldMonitor = async () => enabled && permittedOS.includes(await currentOSPromise);
const monitorEnabled = shouldMonitor();

ProcessMonitor.startMonitor = async (startPID, processInformation) => {
	if (!(await monitorEnabled)) return;
	pidSet.add(startPID);
	informationDict[startPID] = processInformation;
	const currentMemUsage = await getCurrentMemUsage();
	workingDict[startPID] = { startMemory: currentMemUsage, maxMemory: currentMemUsage };
	monitor();
	logger.verbose(`Monitoring enabled for process ${startPID} starting at ${workingDict[startPID].startMemory}`, logLabel);
};

ProcessMonitor.stopMonitor = async (stopPID, returnCode) => {
	if (!(await monitorEnabled)) return;
	// take the PID out of circulation for checking immediately.
	pidSet.delete(stopPID);

	// add the missing properties for sending.
	informationDict[stopPID].ReturnCode = returnCode;
	informationDict[stopPID].MaxMemory = workingDict[stopPID].maxMemory - workingDict[stopPID].startMemory;
	logger.verbose(`${stopPID} ${workingDict[stopPID].maxMemory} - ${workingDict[stopPID].startMemory} = ${informationDict[stopPID].MaxMemory}`, logLabel);
	informationDict[stopPID].ProcessTime = workingDict[stopPID].elapsedTime;

	if (elasticEnabled) {
		try {
			await Elastic.createProcessRecord(informationDict[stopPID]);
		} catch (err) {
			logger.error('Failed to create record', logLabel);
		}
	} else {
		logger.info(`${stopPID} stats ProcessTime: ${informationDict[stopPID].ProcessTime} MaxMemory: ${informationDict[stopPID].MaxMemory}`, logLabel);
	}

	delete informationDict[stopPID];
	delete workingDict[stopPID];
};

module.exports = ProcessMonitor;
