const pidusage = require('pidusage');
const processExists = require('process-exists');
const fs = require('fs');
const logger = require('./logger');
const { elastic } = require('./config').config;

const processMonitor = {};
processMonitor.logLabel = { label: 'PROCESSMON' };

const pidArray = [];
let stats = [];
let maxMemory = 0;
let startMemory = 0;

processMonitor.maxmem = async () => {
	try {
		const data = Number(fs.readFileSync('/sys/fs/cgroup/memory/memory.usage_in_bytes'));
		if (pidArray.length > 0) {
			stats = await pidusage(pidArray);
			if (data > maxMemory) maxMemory = data;
		}
		} catch (err) { logger.verbose(`[processMonitor][maxmem][error]: ${err}`, processMonitor.logLabel); }
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
	startMemory = Number(fs.readFileSync('/sys/fs/cgroup/memory/memory.usage_in_bytes'));
	processMonitor.monitor();
	logger.verbose(`[${inputPID}]: a startMonitor event occurred!`, processMonitor.logLabel);
};

processMonitor.stopMonitor = async (inputPID) => {
	const index = pidArray.indexOf(inputPID);
	if (index !== -1) {
		pidArray.splice(index, 1);
	}
	processMonitor.maxMemory = maxMemory - startMemory;
	processMonitor.processTime = stats[inputPID].elapsed;
	logger.verbose(`[${inputPID}]: a stopMonitor event occurred! elapsed: ${processMonitor.processTime}`, processMonitor.logLabel);
	startMemory = 0;
	maxMemory = 0;
};

module.exports = processMonitor;
