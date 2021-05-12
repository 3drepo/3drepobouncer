const pidusage = require('pidusage');
const processExists = require('process-exists');
const si = require('systeminformation');
const fs = require('fs');
const logger = require('./logger');
const Elastic = require('../lib/elastic');
const { enabled, memoryIntervalMS } = require('./config').config.elastic;

const ProcessMonitor = {};
const logLabel = { label: 'ProcessMonitor' };

const informationDict = {}
const workingDict = {}
const pidArray = [];
let stats = [];

// const getCurrentMemUsage = () =>  Number(fs.readFileSync('/sys/fs/cgroup/memory/memory.usage_in_bytes'));

const getCurrentMemUsage = async () => {
	try {
		const data = await si.mem();
		return data.used
	} catch (e) {
		logger.error("Failed to get memory information record", logLabel)
	}
}
const getCurrentOperatingSystem = async () => {
	try {
		const data = await si.osInfo();
		return data.platform
	} catch (e) {
		logger.error("Failed to get operating system information record", logLabel)
	}
}

const maxmem = async () => {
	try {
		const data = getCurrentMemUsage();
		if (pidArray.length > 0) {
			stats = await pidusage(pidArray);
			for (stat in stats){
				logger.verbose(stat, logLabel)
				if (!workingDict[stat.pid] === undefined ){
					if (data > workingDict[stat.pid].maxMemory) workingDict[stat.pid].maxMemory = data
				}
			}
			// if (data > maxMemory) maxMemory = data;
		}
		} catch (err) { logger.verbose(`[maxmem][error]: ${err}`, logLabel); }
};

// Compute statistics every interval:
const interval = async (time) => {
	if (pidArray.length > 0) {
		setTimeout(async () => {
			await maxmem();
			interval(time);
		}, time);
	}
};

const monitor = async () => {
	interval(memoryIntervalMS);
};

ProcessMonitor.startMonitor = async (inputPID, processInformation) => {
	if (currentOS === "linux" ) {
		pidArray.push(inputPID);
		informationDict[inputPID] = processInformation
		workingDict[inputPID] = { startMemory: 0, maxMemory: 0 }
		workingDict[inputPID].startMemory = getCurrentMemUsage();
		monitor();
		logger.verbose(`[${inputPID}]: a startMonitor event occurred!`, logLabel);
	}
};

ProcessMonitor.stopMonitor = async (inputPID) => {
	if (currentOS === "linux" ) {
		if (!informationDict[inputPID] === undefined ){

			informationDict[inputPID].maxMemory = workingDict[inputPID].maxMemory - workingDict[inputPID].startMemory;
			informationDict[inputPID].processTime = stats[inputPID].elapsed;
			
			workingDict[inputPID] = { startMemory: 0, maxMemory: 0 }

			logger.verbose(`[${inputPID}]: a stopMonitor event occurred! elapsed: ${informationDict[inputPID].processTime}`, logLabel);
			if (enabled) {
				try { 
					Elastic.createRecord (informationDict[inputPID])
				} catch (err){
					logger.error("Failed to create record", logLabel)
				}
			}

			const index = pidArray.indexOf(inputPID);
			if (index !== -1) {
				pidArray.splice(index, 1);
			}

			delete informationDict[inputPID]
			delete workingDict[inputPID]
		}
	}
};

const currentOS = getCurrentOperatingSystem()

module.exports = ProcessMonitor;
