#include "net/LengthHeaderCodec.h"
#include "base/Logging.h"
#include "net/Buffer.h"
#include "net/Endian.h"

namespace net
{

LengthHeaderCodec::LengthHeaderCodec(const MessageCallback& cb)
	: messageCallback_(cb)
{
}

void LengthHeaderCodec::onMessage(const TcpConnectionPtr& conn, Buffer* buffer)
{
	while (buffer->length() >= kHeaderLen)
	{
		const int32_t len = buffer->peekInt32();
		if (len > static_cast<int32_t>(kMessageMaxLen) || len < 0)
		{
			LOG_ERROR << "Invalid length " << len;
			conn->close();
			break;
		}
		else if (buffer->length() >= len + kHeaderLen)
		{
			buffer->retrieve(kHeaderLen);
			Buffer message;
			message.removeBuffer(buffer, len);
			messageCallback_(conn, &message);
		}
		else
		{
			break;
		}
	}
}

void LengthHeaderCodec::send(const TcpConnectionPtr& conn, const net::BufferPtr& message)
{
	int32_t slen = static_cast<int32_t>(message->length());
	int32_t be32 = sockets::hostToNetwork32(slen);
	message->prepend(&be32, sizeof(be32));
	conn->send(message);
}

void LengthHeaderCodec::send(const TcpConnectionPtr& conn, const char* message, size_t len)
{
	BufferPtr buffer(new Buffer());
	buffer->append(message, len);
	send(conn, buffer);
}

void LengthHeaderCodec::send(const TcpConnectionPtr& conn, const std::string& message)
{
	send(conn, message.data(), message.size());
}

} // namespace net
