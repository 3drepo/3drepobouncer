module.exports = {
	rabbitmq:{
		host: 'amqp://localhost:5672',
		worker_queue: 'jobq'
	},
	bouncer:{
		path : 'C:/local/3drepobouncer/bin/repobouncerclient.exe',
		dbhost :'localhost',
		dbport: 27017,
		username : 'username',
		password : 'password'
	}

}
