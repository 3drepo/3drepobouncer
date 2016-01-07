#!/usr/bin/env_node

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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/**
 * Test for bouncer_worker.js
 * Use this as a client to submit work to the queue
 * usage: node test.js <command> <command args>
 */

var amqp = require('amqplib/callback_api');
var conf = require('./config.js')


function generateUuid() {
  return Math.random().toString() +
         Math.random().toString() +
         Math.random().toString();
}

amqp.connect(conf.rabbitmq.host, function(err,conn){
	conn.createChannel(function(err,ch){
		var msg = process.argv.slice(2).join(' ') || "Hello World";
		
		ch.assertQueue('', {exclusive: true}, function(err, q){
			var corr = generateUuid();
			ch.consume(q.queue, function(msg){

			console.log('[.] Got %s', msg.content.toString());
			setTimeout(function() {conn.close(); process.exit(0)},500);	
				
			
			}, {noAck: true});
			ch.sendToQueue(conf.rabbitmq.worker_queue, new Buffer(msg),{correlationId: corr, replyTo:q.queue, persistent: true});
			console.log(" [x] Sent '%s'", msg);	

		});
		

	});
});
