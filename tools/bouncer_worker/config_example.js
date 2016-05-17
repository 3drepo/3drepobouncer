module.exports = {
	rabbitmq:{
		host: 'amqp://username:password@localhost:5672',
		worker_queue: 'jobq'
		callback_queue: 'callbackq'
	},
	bouncer:{
		path : 'C:/local/3drepobouncer/bin/repobouncerclient.exe',
		dbhost :'localhost',
		dbport: 27017,
		username : 'username',
		password : 'password',
		log_dir : '/var/log/3drepo/bouncer/';
	}

}
