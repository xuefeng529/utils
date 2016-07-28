#include "net/InetAddress.h"
#include "base/Logging.h"

#include <stdio.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

namespace net
{

InetAddress::InetAddress(uint16_t port)
{
	bzero(&addr_, sizeof(addr_));
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = sockets::hostToNetwork32(INADDR_ANY);
	addr_.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(char* ip, uint16_t port)
{
	bzero(&addr_, sizeof(addr_));
	addr_.sin_family = AF_INET;
	addr_.sin_port = sockets::hostToNetwork16(port);
	if (::inet_pton(AF_INET, ip, &addr_.sin_addr.s_addr) <= 0)
	{
		char errorBuf[512];
		strerror_r(errno, errorBuf, sizeof(errorBuf));
		LOG_SYSERR << "inet_pton error: " << errorBuf;
	}
}

std::string InetAddress::toIpPort() const
{
  char buf[64] = "";
  ::inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, static_cast<socklen_t>(sizeof(buf)));
  size_t end = ::strlen(buf);
  uint16_t port = sockets::networkToHost16(addr_.sin_port);
  snprintf(buf + end, sizeof(buf) - end, ":%u", port);
  return buf;
}

std::string InetAddress::toIp() const
{
	char buf[64] = "";
	::inet_ntop(AF_INET, &addr_.sin_addr.s_addr, buf, static_cast<socklen_t>(sizeof(buf)));
	return buf;
}

static __thread char t_resolveBuffer[64 * 1024];

bool InetAddress::resolve(const char* hostname, InetAddress* out)
{
  struct hostent hent;
  struct hostent* he = NULL;
  int herrno = 0;
  bzero(&hent, sizeof(hent));

  int ret = gethostbyname_r(hostname, &hent, t_resolveBuffer, sizeof(t_resolveBuffer), &he, &herrno);
  if (ret == 0 && he != NULL)
  {
    assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
	out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
    return true;
  }
  else
  {
    if (ret)
    {
		char errorBuf[512];
		strerror_r(errno, errorBuf, sizeof(errorBuf));
		LOG_SYSERR << "InetAddress::resolve error: " << errorBuf;
    }
    return false;
  }
}

} // namespace net
