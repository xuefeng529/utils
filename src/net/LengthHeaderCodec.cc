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
		if (len > 65536 || len < 0)
		{
			LOG_ERROR << "Invalid length " << len;
			conn->shutdown();
			break;
		}
		else if (buffer->length() >= len + kHeaderLen)
		{
			buffer->retrieve(kHeaderLen);
			Buffer message;
			buffer->removeBuffer(&message, len);
			messageCallback_(conn, &message);
		}
		else
		{
			break;
		}
	}
}

void LengthHeaderCodec::send(const TcpConnectionPtr& conn, const char* message, size_t len)
{
	BufferPtr buffer(new Buffer());
	buffer->append(message, len);
	int32_t slen = static_cast<int32_t>(len);
	int32_t be32 = sockets::hostToNetwork32(slen);
	buffer->prepend(&be32, sizeof(be32));
	conn->send(buffer);
}

void LengthHeaderCodec::send(const TcpConnectionPtr& conn, const std::string& message)
{
	send(conn, message.data(), message.size());
}

} // namespace net
