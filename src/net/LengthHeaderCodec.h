#ifndef NET_LENGTH_HEADER_CODEC_H
#define NET_LENGTH_HEADER_CODEC_H

#include "net/TcpConnection.h"

namespace net
{

class LengthHeaderCodec : boost::noncopyable
{
public:
	typedef boost::function<void(const TcpConnectionPtr&, Buffer* buffer)> MessageCallback;

	explicit LengthHeaderCodec(const MessageCallback& cb);

	void onMessage(const TcpConnectionPtr& conn, Buffer* buffer);

	void send(const TcpConnectionPtr& conn, const char* message, size_t len);
	void send(const TcpConnectionPtr& conn, const std::string& message);

private:
	const static size_t kHeaderLen = sizeof(int32_t);

	MessageCallback messageCallback_;
};

} // namespace net

#endif  // NET_LENGTH_HEADER_CODEC_H
