#!/usr/bin/env node

/**
 *  Copyright (C) 2015 3D Repo Ltd
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 * /

 /*
  * Bouncer Worker - The RabbitMQ Client for bouncer library
  * To run this:
  *  - ensure NodeJS/npm is installed
  *  - create config.js (use config_example.js as your base)
  *  - npm install amqplib
  *  - node bouncer_worker.js
  */


var amqp = require('amqplib/callback_api')
var conf = require('./config.js')
var path = require('path');
var exec = require('child_process').exec;


/**
 * Test that the client is working and
 * it is able to connect to the database
 * Takes a callback function to with 
 * indication of success or failure
 */
function testClient(callback){
	console.log("Checking status of client...");
	exec(path.normalize(conf.bouncer.path) + ' ' + conf.bouncer.dbhost + ' ' + conf.bouncer.dbport + ' ' + conf.bouncer.username + ' ' + conf.bouncer.password + ' test', function(error, stdout, stderr){
		if(error !== null){
			console.log(stdout);
		}
		else{
			callback();
		}
	});

}


/**
 * Execute the Command and provide a reply message to the callback function
 */
function exeCommand(cmd, callback){


	exec(path.normalize(conf.bouncer.path) + ' ' + conf.bouncer.dbhost + ' ' + conf.bouncer.dbport + ' ' + conf.bouncer.username + ' ' + conf.bouncer.password + ' ' + cmd, function(error, stdout, stderr){
		var reply = "Failed";
		if(error == null){
			reply = "Success";
		}
		console.log("Executed command: %s", reply);
		callback(reply);
	});
}

function connectQ(){
	amqp.connect(conf.rabbitmq.host, function(err,conn){
		conn.createChannel(function(err, ch){
			ch.assertQueue(conf.rabbitmq.worker_queue, {durable: true});
			console.log("Bouncer Client Queue started. Waiting for messages in %s of %s....", conf.rabbitmq.worker_queue, conf.rabbitmq.host);
			ch.prefetch(1);
			ch.consume(conf.rabbitmq.worker_queue, function(msg){
				console.log(" [x] Received %s", msg.content.toString());
				exeCommand(msg.content.toString(), function(reply){
					console.log("sending to reply queue(%s): %s", msg.properties.replyTo, reply);
					ch.sendToQueue(msg.properties.replyTo, new Buffer(reply),
						{correlationId: msg.properties.correlationId});	
				});		
			}, {noAck: true});
		});
	});
}


console.log("Initialising bouncer client queue...");
testClient(connectQ);


