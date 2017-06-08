module.exports = {
	rabbitmq:{
		host: 'amqp://username:password@localhost:5672',
		worker_queue: 'jobq'
		callback_queue: 'callbackq',
		prefetch: 1
	},
	bouncer:{
		path : 'C:/local/3drepobouncer/bin/repobouncerclient.exe',
		dbhost :'localhost',
		dbport: 27017,
		username : 'username',
		password : 'password',
		log_dir : '/var/log/3drepo/bouncer/'
	},
	mongoimport:{
		writeConcern: {
			w: 1
		},
	},
	unity:
	{
		project: "C:\\unity.bat",
		batPath: 'buildBundle.bat'
	}

}
