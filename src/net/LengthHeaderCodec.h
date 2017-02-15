#ifndef NET_LENGTHHEADERCODEC_H
#define NET_LENGTHHEADERCODEC_H

#include "net/TcpConnection.h"

namespace net
{

class LengthHeaderCodec : boost::noncopyable
{
public:
	typedef boost::function<void(const TcpConnectionPtr&, Buffer* buffer)> MessageCallback;

	explicit LengthHeaderCodec(const MessageCallback& cb);

	void onMessage(const TcpConnectionPtr& conn, Buffer* buffer);

	void send(const TcpConnectionPtr& conn, const net::BufferPtr& message);
	void send(const TcpConnectionPtr& conn, const char* message, size_t len);
	void send(const TcpConnectionPtr& conn, const std::string& message);

private:
	const static size_t kHeaderLen = sizeof(int32_t);
	const static size_t kMessageMaxLen = 4 * 1024 - kHeaderLen;

	MessageCallback messageCallback_;
};

} // namespace net

#endif  // NET_LENGTHHEADERCODEC_H
