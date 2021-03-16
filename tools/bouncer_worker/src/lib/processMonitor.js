const pidusage = require('pidusage');
const processExists = require('process-exists');
const logger = require('./logger');
const { elastic } = require('./config').config;
const fs = require('fs');

const processMonitor = {};
processMonitor.logLabel = { label: 'PROCESSMON' };

let pidArray = [];
let stats = [];
let maxMemory = 0;
let startMemory = 0;

processMonitor.maxmem = async () => {
	try {
		// logger.debug(("[processMonitor]:maxmem: " + Date.now(),"pidArray",pidArray),processMonitor.logLabel)
		let data = Number(fs.readFileSync('/sys/fs/cgroup/memory/memory.usage_in_bytes'))
		if (pidArray.length > 0) {
			stats = await pidusage(pidArray);
			if (data > maxMemory) maxMemory = data
			// console.log(stats)
			// logger.debug(stats,processMonitor.logLabel)
		}
	} catch (err) { logger.verbose(`[maxmem][error]: ${err}`, processMonitor.logLabel); }
};

// Compute statistics every interval:
processMonitor.interval = async (time) => {
	const exists = await processExists.all(pidArray);
	if (pidArray.length > 0 && exists) {
		setTimeout(async () => {
			await processMonitor.maxmem();
			processMonitor.interval(time);
		}, time);
	}
};

processMonitor.monitor = async () => {
	processMonitor.interval(elastic.memoryIntervalMS);
};

processMonitor.startMonitor = async (inputPID) => {
	pidArray.push(inputPID);
	startMemory = Number(fs.readFileSync('/sys/fs/cgroup/memory/memory.usage_in_bytes'))
	processMonitor.monitor();
	logger.verbose(`[${inputPID}]: a startMonitor event occurred!`, processMonitor.logLabel);
};

processMonitor.stopMonitor = async (inputPID) => {
	// console.log(stats)
	var index = pidArray.indexOf(inputPID);
	if (index !== -1) {
		pidArray.splice(index, 1);
	}
	processMonitor.maxMemory = maxMemory - startMemory; // stats[inputPID].memory;
	processMonitor.processTime = stats[inputPID].elapsed;
	logger.verbose(`[${inputPID}]: a stopMonitor event occurred! elapsed: ${processMonitor.processTime}`, processMonitor.logLabel);
	// stats = [];
	startMemory = 0 
	maxMemory = 0
};

module.exports = processMonitor;
