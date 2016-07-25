#ifndef PLUGIN_HASH_CONSISTENT_HASH_H
#define PLUGIN_HASH_CONSISTENT_HASH_H

#include "plugins/consistent_hash/conhash.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>

namespace plugin
{
namespace hash
{

class ConsistentHash : boost::noncopyable
{
public:
	static const size_t kVirtualNodeNum = 32;

	ConsistentHash();
	~ConsistentHash();

	bool addNode(const std::string& name, size_t numVirtualNodes = kVirtualNodeNum);
	bool deleteNode(const std::string& name);
	const char* lookup(const std::string& object) const;

private:
	struct conhash_s* conHash_;
	typedef struct node_s Node;
	typedef boost::shared_ptr<Node> NodePtr;
	std::map<std::string, NodePtr> nodes_;
};

} // namespace hash
} // namespace plugin

#endif // PLUGIN_HASH_CONSISTENT_HASH_H
