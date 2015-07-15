#include "repo_graph_abstract.h"

using namespace repo::manipulator::graph;

AbstractGraph::AbstractGraph() :
rootNode(0)
{
}

AbstractGraph::AbstractGraph(
	repo::core::handler::AbstractDatabaseHandler *dbHandler,
	const std::string &databaseName,
	const std::string &projectName) :
	dbHandler(dbHandler),
	databaseName(databaseName),
	projectName(projectName),
	rootNode(0)
{

}

AbstractGraph::~AbstractGraph()
{
}


//bool append(
//	std::vector<repo::core::model::bson::RepoBSON> nodes)
//{
//
//
//}
