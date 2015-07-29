#include "repo_graph_abstract.h"

using namespace repo::manipulator::graph;

AbstractGraph::AbstractGraph(
	const std::string &databaseName,
	const std::string &projectName) :
	databaseName(databaseName),
	projectName(projectName),
	rootNode(0)
{

}

AbstractGraph::~AbstractGraph()
{
}

