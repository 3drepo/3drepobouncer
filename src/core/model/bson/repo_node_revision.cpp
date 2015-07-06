#include "repo_node_revision.h"

using namespace repo::core::model::bson;


RevisionNode::RevisionNode(RepoBSON bson) :
	RepoNode(bson)
{

}
RevisionNode::RevisionNode() :
	RepoNode()
{
}


RevisionNode::~RevisionNode()
{
}
