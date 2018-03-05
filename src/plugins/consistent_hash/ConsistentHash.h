#ifndef PLUGINS_CONSISTENTHASH_H
#define PLUGINS_CONSISTENTHASH_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>

struct conhash_s;
struct node_s;

namespace plugins
{

class ConsistentHash : boost::noncopyable
{
public:
	static const size_t kNumVirtualNodes = 32;

	ConsistentHash();
	~ConsistentHash();

    bool addNode(const std::string& name, size_t numVirtualNodes = kNumVirtualNodes);
	bool deleteNode(const std::string& name);
	const char* lookup(const std::string& object) const;

private:
	struct conhash_s* conHash_;
	typedef struct node_s Node;
	typedef boost::shared_ptr<Node> NodePtr;
	std::map<std::string, NodePtr> nodes_;
};

} // namespace plugins

#endif // PLUGINS_CONSISTENTHASH_H
