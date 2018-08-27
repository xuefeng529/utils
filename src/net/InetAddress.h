#ifndef NET_INETADDRESS_H
#define NET_INETADDRESS_H

#include "Endian.h"

#include <string>

#include <netinet/in.h>

namespace net
{

class InetAddress
{
public:
	explicit InetAddress(uint16_t port);
	InetAddress(const char* ip, uint16_t port);
	explicit InetAddress(const struct sockaddr_in& addr)
		: addr_(addr)
	{ }
	
	std::string toIp() const;
	std::string toIpPort() const;
	uint16_t toPort() const
	{ return sockets::networkToHost16(portNetEndian()); }

	const struct sockaddr* getSockAddr() const
	{ return reinterpret_cast<const struct sockaddr*>(&addr_); }

	uint32_t ipNetEndian() const
	{ return addr_.sin_addr.s_addr; }

	uint16_t portNetEndian() const
	{ return addr_.sin_port; }

	static bool resolve(const char* hostname, InetAddress* result);

private:
	struct sockaddr_in addr_;
};

} // namespace net

#endif  // NET_INETADDRESS_H
