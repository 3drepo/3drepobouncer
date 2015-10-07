#include "repo_graph_abstract.h"

using namespace repo::core::model;

AbstractGraph::AbstractGraph(
	const std::string &databaseName,
	const std::string &projectName) :
	databaseName(sanitizeDatabaseName(databaseName)),
	projectName(sanitizeName(projectName))
{

}

AbstractGraph::~AbstractGraph()
{
}

