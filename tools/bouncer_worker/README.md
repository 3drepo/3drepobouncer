## About Bouncer Worker
Bouncer worker is a NodeJS client that handles communications between 3drepo.io and 3drepobouncer. It connects to a queue via ampq protocol, parses the incoming messages and operate on them accordingly.
It performs 3 notable actions:
  1. Import sample model via mongoimport/export
  2. Create federations
  3. Import model from model file.

## Getting Started
### Prerequisites
  - [NodeJS v10](https://nodejs.org/dist/latest-v10.x/)
  - 3drepobouncerClient compiled
  - 3drepobouncerwrapper compiled
  - mongoexport utility
  - Access to 3drepounity
  - Access to Toy Model export
### Installation
  - `yarn install`

## Usage
  To run Bouncer Worker with default settings:
  ````
  yarn run-worker
  ````
  This will run bouncer worker, listening to all configured queues, specied in the default config location (`./config.json`)

  Use `--config` to specify the path to your configuration file
  ````
  yarn run-worker --config prod_config.json
  ````

  `--queue [job|model|unity]` will only listen to the specified queue
  ````
  yarn run-worker --queue unity
  ````

  `--exitAfter <number>` will process the specified amount of tasks on the queue and exit the process. note: `--queue` must be specified in this mode.
  ````
  yarn run-worker --queue unity --exitAfter 10
  ````

  Run with `--help` to see the list of available options
  ````
  yarn run-worker --help
  ````

## Configurations
Bouncer worker takes in a JSON file with the following parameters.

````js
{
  "umask": //umask to set at initialisation (default: none)
  "timeoutMS": //number of ms to spend on a task before timing out (default: 3hours)
  "logging" : {
  	"logLevel": //Only show logging of this severity level or above (default: info)
    "taskLogDir": //Path to where the individual task logs should go for each tasks (Used to be in bouncer.log_dir). (default: rabbitmq.sharedDir[recommended])
    "workerLogPath": //Where bouncer worker will write it log. If it is not specified, it will not log to file.
  },
  "rabbitmq": {
    "host" : //URL to rabbitmq server
    "worker_queue": //name of the worker queue
    "callback_queue": //name of the callback queue
    "model_queue": //name of the model queue
    "unity_queue": //name of the unity queue
    "task_prefetch": //Number of tasks to process simultaneously on the worker queue (default: 4)
    "model_prefetch": //Number of tasks to process simultaneously on the model queue (default: 1)
    "sharedDir": //Path to the sharedSpace being referenced by the queue
    "maxRetries": //Maximum number of attempts to reconnect to the queue before giving up (default: 3)
    "waitBeforeShutdownMS": //In exitAfter mode, the number of ms to wait before shutting down the application (default: 60000)
    "pollingIntervalMS" : //In exitAfter mode, the number of ms to wait inbetween polling the queue for tasks (default: 10000)
    "maxWaitTimeMS" : //In exitAfter mode, the maximum wait time for a task before exiting the process (default: 300000 - aka 5mins)
  },
  "bouncer": {
    "path": //path to 3drepobouncerClient
    "envar": { } //additional envars to set whilst running 3drepobouncerClient
  },
  "db": {
    "dbhost": //hostname of mongo database
    "dbport": //port of mongo database
    "username": //authentication username
    "password": //authentication password
  },
  "fs": {
    //fs configuration is entirely optional.
    "path" : //path to fileshare
    "level": //number of levels to split
  },
  "unity": {
    "project": //location of AssetBundleCreator project
    "batPath": //path to buildBundle script.
  },
  "repoLicense": //license string for license check (only required on builds that are license enforced)
  "instanceID": //identification string to identify the machine instance (only used when repoLicense is set to true, optional - will auto generate if not given)
}
````
