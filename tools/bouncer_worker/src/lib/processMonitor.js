const pidusage = require('pidusage');
const processExists = require('process-exists');
const logger = require('./logger');
const { elastic } = require('./config').config;

let maxMemoryConsumption = 0;
let maxTimer = 0;

const processMonitor = {};
processMonitor.logLabel = { label: 'processMonitor' };
processMonitor.maxmem = async (inputPID, exists) => {
	try {
		if (exists) {
			const stats = await pidusage(inputPID);
			maxTimer = stats.elapsed;
			logger.debug(`[processMonitor]: current: ${stats.memory} time: ${stats.elapsed} max ${maxMemoryConsumption}`, 'pidusage');
			if (stats.memory > maxMemoryConsumption) {
				maxMemoryConsumption = stats.memory;
				logger.debug(`[processMonitor]: new Maximum: ${maxMemoryConsumption}`, processMonitor.logLabel);
			}
		}
	} catch (err) { logger.verbose(`[ERROR]: ${err}`, processMonitor.logLabel); }
};

// Compute statistics every interval:
processMonitor.interval = async (time, inputPID) => {
	const exists = await processExists(inputPID);
	setTimeout(async () => {
		await processMonitor.maxmem(inputPID, exists);
		processMonitor.interval(time);
	}, time);
};

processMonitor.monitor = async (inputPID) => {
	processMonitor.interval(elastic.memoryIntervalMS, inputPID);
};

processMonitor.startMonitor = async (inputPID) => {
	maxTimer = 0;
	maxMemoryConsumption = 0;
	processMonitor.monitor(inputPID);
	logger.verbose(`[${inputPID}]: a startMonitor event occurred!`, processMonitor.logLabel);
};

processMonitor.stopMonitor = async (inputPID) => {
	logger.verbose(`[${inputPID}]: a stopMonitor event occurred! ${maxMemoryConsumption}`, processMonitor.logLabel);
	processMonitor.maxMemory = maxMemoryConsumption;
	processMonitor.processTime = maxTimer;
};

module.exports = processMonitor;