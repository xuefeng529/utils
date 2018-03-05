#include "plugins/consistent_hash/ConsistentHash.h"
#include "plugins/consistent_hash/conhash.h"
#include "base/Logging.h"

#include <stdio.h>

namespace plugins
{

const size_t ConsistentHash::kNumVirtualNodes;

ConsistentHash::ConsistentHash()
	: conHash_(conhash_init(NULL))
{
	if (conHash_ == NULL)
	{
		LOG_FATAL << "conhash_init failed !";
	}
}

ConsistentHash::~ConsistentHash()
{
	conhash_fini(conHash_);
}

bool ConsistentHash::addNode(const std::string& name, size_t numVirtualNodes)
{
	std::map<std::string, NodePtr>::const_iterator it = nodes_.find(name);
	if (it != nodes_.end())
	{
		return false;
	}
	
	NodePtr node(new Node());
	conhash_set_node(node.get(), name.c_str(), numVirtualNodes);
	if (conhash_add_node(conHash_, node.get()) == -1)
	{
		return false;
	}

	nodes_.insert(std::make_pair(name, node));
	return true;
}

bool ConsistentHash::deleteNode(const std::string& name)
{
	std::map<std::string, NodePtr>::iterator it = nodes_.find(name);
	if (it == nodes_.end())
	{
		return false;
	}
	
	NodePtr node = it->second;
	nodes_.erase(name);
	return (conhash_del_node(conHash_, node.get()) == 0);
}

const char* ConsistentHash::lookup(const std::string& object) const
{
    const Node* node = conhash_lookup(conHash_, object.c_str());
	return (node != NULL ? node->iden : NULL);
}

} // namespace plugins
