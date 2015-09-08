#include <iostream>
#include <list>
#include <sstream>

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <repo/repo_controller.h>

#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#include <Windows.h>

#endif

#include <mongo/client/dbclient.h>


void configureLogging(){

	boost::log::core::get()->set_filter
	(
		boost::log::trivial::severity >= boost::log::trivial::info //only show info and above
	);
}

void testDatabaseRetrieval(repo::RepoController *controller, const repo::RepoToken *token)
{
	repoInfo << "Fetching list of databases...";
	std::list<std::string> databases = controller->getDatabases(token);
	repoInfo << "Found " << databases.size() << " databases.";
	std::string dbList;

	std::list<std::string>::iterator it;
	for (it = databases.begin(); it != databases.end(); ++it)
	{
		if (it != databases.begin())
			dbList += ",";
		dbList += *it;
	}

	repoInfo << "Database List: {" << dbList << "}";


	if (databases.size() > 0)
	{
		std::string colList;

		std::string dbName = *databases.begin();
		repoInfo << "Fetching list of collections in " << dbName << " ...";
		std::list<std::string> collections = controller->getCollections(token, dbName);
		repoInfo << "Found " << collections.size() << " collections.";
		
		for (it = collections.begin(); it != collections.end(); ++it)
		{
			if (it != collections.begin())
				colList += ",";
			colList += *it;
		}
		repoInfo << "Collection List: {" << colList << "}";

	}
}

class TestListener : public repo::lib::RepoAbstractListener
{
public:
	TestListener() {}
	~TestListener(){}

	void messageGenerated(const std::string &message)
	{
		std::cout << "[LISTENER] " << message;
	}
};

void testDeletion(repo::RepoController *controller, repo::RepoToken *token)
{
	std::cout << "Trying to delete collection : deleteME";
	std::string errMsg;
	if (controller->removeCollection(token, "deleteME", "deleteME", errMsg))
	{
		std::cout << "Collection Deleted!" << std::endl;
	}
	else
	{
		std::cerr << "Failed to delete collection : " << errMsg;
	}

	std::cout << "Trying to delete database : deleteME";

	if (controller->removeDatabase(token, "deleteME", errMsg))
	{
		std::cout << "Database Deleted!" << std::endl;
	}
	else
	{
		std::cerr << "Failed to delete database : " << errMsg;
	}
}

void loadModelFromFileAndCommit(repo::RepoController *controller, const repo::RepoToken *token)
{
	
	std::string fileName;

	//fileName = "C:\\Users\\Carmen\\Desktop\\models\\A556-CAP-7000-S06-IE-S-1001.ifc"; - no worky at the moment
	//fileName = "C:\\Users\\Carmen\\Desktop\\models\\chair\\Bo Concept Imola.obj";
	//fileName = "C:\\Users\\Carmen\\Desktop\\models\\Duplex_A_20110907.ifc";
	fileName = "C:\\Users\\Carmen\\Desktop\\models\\103EW-A-BASEMENT.fbx";

	repo::manipulator::modelconvertor::ModelImportConfig config;

	config.setPreTransformVertices(true);
	config.setRemoveRedundantMaterials(true);

	repo::core::model::RepoScene *graph = nullptr;
	try{
		graph = controller->loadSceneFromFile(fileName, &config);
	}
	catch (std::exception &e)
	{
		std::cout << "Exception occured whilst loading scene from file: " << e.what() <<std::endl;
	}

	if (graph)
	{
		BOOST_LOG_TRIVIAL(info) << "model loaded successfully! Attempting to port to Repo World...";

		BOOST_LOG_TRIVIAL(info) << "RepoScene generated. Printing graph statistics...";
		std::stringstream		stringMaker;
		graph->printStatistics(stringMaker);
		std::cout << stringMaker.str();

		std::string databaseName = "test";
		std::string projectName = "bigfileTest";
		//BOOST_LOG_TRIVIAL(info) << "Trying to commit this scene to database as " << databaseName << "." << projectName;
		//
		graph->setDatabaseAndProjectName(databaseName, projectName);

		controller->commitScene(token, graph);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to load model from file : " << fileName  ;
	}


}


int main(int argc, char* argv[]){

	//TODO: configuration needs to be done properly, but hey, i'm just a quick test!
	if (argc != 5){
		std::cout << "Usage: " << std::endl;
		std::cout << "\t " << argv[0] << " address port username password" << std::endl;
		return EXIT_FAILURE;
	}

	std::string address = argv[1];
	int port = atoi(argv[2]);
	std::string username = argv[3];
	std::string password = argv[4];


	//mongo::client::initialize();
	//mongo::HostAndPort hostAndPort = mongo::HostAndPort(address, port >= 0 ? port : -1);
	//mongo::ConnectionString cs = mongo::ConnectionString(hostAndPort);
	//mongo::DBClientBase *worker = cs.connect(std::string());

	//std::string passwordDigest =  mongo::DBClientWithCommands::createPasswordDigest(username, password);
	//mongo::BSONObj authBson = mongo::BSONObj(BSON("user" << username <<
	//	"db" << "admin" <<
	//	"pwd" << passwordDigest <<
	//	"digestPassword" << false <<
	//	"mechanism" << "MONGODB-CR"));
	//worker->auth(authBson);

	//size_t size = 104857600; //100MB
	//std::string fileName = "testingFile";
	//char* random = (char*)malloc(sizeof(*random) * size);
	//if (random)
	//{
	//	try{
	//		mongo::GridFS gfs(*worker, "test", "gridTest");
	//		mongo::BSONObj bson = gfs.storeFile(random, size, fileName );
	//		std::cout << bson.toString() << std::endl;
	//		mongo::GridFile tmpFile = gfs.findFileByName(fileName);
	//		if (tmpFile.exists())
	//		{
	//			std::cout << "Found file" << std::endl;
	//			std::ostringstream oss;
	//			tmpFile.write(oss);

	//			std::string fileStr = oss.str();
	//			char* random_out = (char*)malloc(sizeof(*fileStr.c_str())*fileStr.size());
	//			if (random_out)
	//			{
	//				std::cout << "fileStr.size = " << fileStr.size();
	//				memcpy(random_out, fileStr.c_str(), fileStr.size());
	//				fflush(stdout);
	//				if (!strcmp(random, random_out))
	//				{
	//					std::cout << " In/Out Matched" << std::endl;

	//				}
	//				else
	//				{
	//					std::cout << "In/Out misMatched" << std::endl;
	//				}
	//			}
	//			else
	//			{
	//				std::cout << "oops... OOM part 2" << std::endl;
	//			}
	//		}
	//	}
	//	catch (std::exception &e)
	//	{
	//		std::cout << "Exception caught: " << e.what();
	//	}
	//	
	//}
	//else
	//{
	//	std::cout << "oops... OOM." << std::endl;
	//}

	//return 0;

	repo::RepoController *controller = new repo::RepoController();

	std::string errMsg;
	repo::RepoToken* token = controller->authenticateToAdminDatabaseMongo(errMsg, address, port, username, password);
	if (token)
		std::cout << "successfully connected to the database!" << std::endl;
	else
		std::cerr << "Failed to authenticate to the database: " << errMsg << std::endl;

	//testDatabaseRetrieval(controller, token);
	//errMsg.clear();

	////testDeletion(controller, token);

	////<< " bsons. first bson has a type of " << bsons.at(0).getStringField(REPO_NODE_LABEL_TYPE);


	//////insertARepoNode(dbHandler);

	//loadModelFromFileAndCommit(controller, token);

	////instantiateProject(dbHandler);
	repo::core::model::RepoScene *scene = controller->fetchScene(token, "test", "bigfileTest");
	std::stringstream		stringMaker;
	scene->printStatistics(stringMaker);
	std::cout << stringMaker.str();
	//controller->saveSceneToFile("C:/Users/Carmen/Desktop/camTest.dae", scene);


	return EXIT_SUCCESS;
}

