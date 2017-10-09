#include "net/Buffer.h"
#include "net/config.h"
#include "base/Logging.h"

#include <stdlib.h>
#include <errno.h>

namespace net
{

Buffer::Buffer()
	: buffer_(evbuffer_new()),
	  bufferOwner_(true)
{
}

Buffer::Buffer(struct evbuffer* buffer)
	: buffer_(buffer),
	  bufferOwner_(false)
{
}

Buffer::~Buffer()
{
	if (bufferOwner_)
	{
		evbuffer_free(buffer_);
	}
}

void Buffer::append(const void* data, size_t len)
{
    if (evbuffer_add(buffer_, data, len) == -1)
    {
        LOG_ERROR << "evbuffer_add error: "
            << evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
    }
}

void Buffer::append(const char* data, size_t len)
{
    append(static_cast<const void*>(data), len);
}

void Buffer::append(const char* str)
{
	append(static_cast<const void*>(str), strlen(str));
}

void Buffer::append(const std::string& str)
{
	append(static_cast<const void*>(str.data()), str.size());
}

void Buffer::appendInt64(int64_t x)
{
	int64_t be64 = sockets::hostToNetwork64(x);
	append(&be64, sizeof(be64));
}

void Buffer::appendInt32(int32_t x)
{
	int32_t be32 = sockets::hostToNetwork32(x);
	append(&be32, sizeof(be32));
}

void Buffer::appendInt16(int16_t x)
{
	int16_t be16 = sockets::hostToNetwork16(x);
	append(&be16, sizeof(be16));
}

void Buffer::appendInt8(int8_t x)
{
	append(&x, sizeof(x));
}

void Buffer::removeBuffer(Buffer* buffer)
{
	evbuffer_remove_buffer(buffer->buffer_, buffer_, buffer->length());
}

void Buffer::removeBuffer(Buffer* buffer, size_t len)
{
	evbuffer_remove_buffer(buffer->buffer_, buffer_, len);
}

void Buffer::appendBuffer(const Buffer& buffer)
{
	std::string data;
	buffer.peekAllAsString(&data);
	append(data);
}

void Buffer::prepend(const void* data, size_t len)
{
    evbuffer_prepend(buffer_, data, len);
}

void Buffer::prependInt64(int64_t x)
{
	int64_t be64 = sockets::hostToNetwork64(x);
	prepend(&be64, sizeof(be64));
}

void Buffer::prependInt32(int32_t x)
{
	int32_t be32 = sockets::hostToNetwork32(x);
	prepend(&be32, sizeof(be32));
}

void Buffer::prependInt16(int16_t x)
{
	int16_t be16 = sockets::hostToNetwork16(x);
	prepend(&be16, sizeof(be16));
}

void Buffer::prependInt8(int8_t x)
{
	prepend(&x, sizeof(x));
}

size_t Buffer::length() const
{
	return evbuffer_get_length(buffer_);
}

int64_t Buffer::readInt64()
{
	int64_t result = peekInt64();
	retrieveInt64();
	return result;
}

int32_t Buffer::readInt32()
{
	int32_t result = peekInt32();
	retrieveInt32();
	return result;
}

int16_t Buffer::readInt16()
{
	int16_t result = peekInt16();
	retrieveInt16();
	return result;
}

int8_t Buffer::readInt8()
{
	int8_t result = peekInt8();
	retrieveInt8();
	return result;
}

int64_t Buffer::peekInt64() const
{	
    if (length() < sizeof(int64_t))
    {
        LOG_ERROR << "invalid buffer length";
        return -1;
    }

    int64_t be64 = 0;
    evbuffer_copyout(buffer_, &be64, sizeof(be64));
	return sockets::networkToHost64(be64);
}

int32_t Buffer::peekInt32() const
{
    if (length() < sizeof(int32_t))
    {
        LOG_ERROR << "invalid buffer length";
        return -1;
    }
	
    int32_t be32 = 0;
    evbuffer_copyout(buffer_, &be32, sizeof(be32));
	return sockets::networkToHost32(be32);
}

int16_t Buffer::peekInt16() const
{
    if (length() < sizeof(int16_t))
    {
        LOG_ERROR << "invalid buffer length";
        return -1;
    }

	int16_t be16 = 0;
	evbuffer_copyout(buffer_, &be16, sizeof(be16));
	return sockets::networkToHost16(be16);
}

int8_t Buffer::peekInt8() const
{
    if (length() < sizeof(int8_t))
    {
        LOG_ERROR << "invalid buffer length";
        return -1;
    }

    int8_t be8 = 0;
    evbuffer_copyout(buffer_, &be8, sizeof(be8));
    return be8;
}

int64_t Buffer::peekInt64WithOriginalEndian() const
{
    if (length() < sizeof(int64_t))
    {
        LOG_ERROR << "invalid buffer length";
        return -1;
    }

	int64_t be64 = 0;
	evbuffer_copyout(buffer_, &be64, sizeof(be64));
	return be64;
}

int32_t Buffer::peekInt32WithOriginalEndian() const
{
    if (length() < sizeof(int32_t))
    {
        LOG_ERROR << "invalid buffer length";
        return -1;
    }

	int32_t be32 = 0;
	evbuffer_copyout(buffer_, &be32, sizeof(be32));
	return be32;
}

int16_t Buffer::peekInt16WithOriginalEndian() const
{
    if (length() < sizeof(int16_t))
    {
        LOG_ERROR << "invalid buffer length";
        return -1;
    }

	int16_t be16 = 0;
	evbuffer_copyout(buffer_, &be16, sizeof(be16));
	return be16;
}

int64_t Buffer::readInt64WithOriginalEndian()
{
	int64_t result = peekInt64WithOriginalEndian();
	retrieveInt64();
	return result;
}

int32_t Buffer::readInt32WithOriginalEndian()
{
	int32_t result = peekInt32WithOriginalEndian();
	retrieveInt32();
	return result;
}

int16_t Buffer::readInt16WithOriginalEndian()
{
	int16_t result = peekInt16WithOriginalEndian();
	retrieveInt16();
	return result;
}

void Buffer::appendInt64WithOriginalEndian(int64_t x)
{
    append(&x, sizeof(x));
}

void Buffer::appendInt32WithOriginalEndian(int32_t x)
{
    append(&x, sizeof(x));
}

void Buffer::appendInt16WithOriginalEndian(int16_t x)
{
    append(&x, sizeof(x));
}

void Buffer::prependInt64WithOriginalEndian(int64_t x)
{
    prepend(&x, sizeof(x));
}

void Buffer::prependInt32WithOriginalEndian(int32_t x)
{
    prepend(&x, sizeof(x));
}

void Buffer::prependInt16WithOriginalEndian(int16_t x)
{
    prepend(&x, sizeof(x));
}

void Buffer::retrieve(size_t len)
{
	if (len < length())
	{
		evbuffer_drain(buffer_, len);
	}
	else
	{
		retrieveAll();
	}
}

void Buffer::retrieveInt64()
{
	retrieve(sizeof(int64_t));
}

void Buffer::retrieveInt32()
{
	retrieve(sizeof(int32_t));
}

void Buffer::retrieveInt16()
{
	retrieve(sizeof(int16_t));
}

void Buffer::retrieveInt8()
{
	retrieve(sizeof(int8_t));
}

void Buffer::Buffer::retrieveAll()
{
	evbuffer_drain(buffer_, evbuffer_get_length(buffer_));
}

void Buffer::retrieveAsBytes(char* buf, size_t len)
{
    if (length() < len)
    {
        LOG_ERROR << "invalid buffer length";
        return;
    }

    evbuffer_remove(buffer_, buf, len);
}

void Buffer::retrieveAsString(size_t len, std::string* ret)
{
    ret->resize(len);
    retrieveAsBytes(&*ret->begin(), ret->size());
}

void Buffer::retrieveAllAsString(std::string* ret)
{
	return retrieveAsString(length(), ret);
}

bool Buffer::readLine(std::string* ret)
{
	char* line = evbuffer_readln(buffer_, NULL, EVBUFFER_EOL_CRLF);
	if (line == NULL)
	{
		return false;
	}

	*ret = line;
	free(line);
	return true;
}

void Buffer::peekAsBytes(char* buf, size_t len) const
{
    if (length() < len)
    {
        LOG_ERROR << "invalid buffer length";
        return;
    }

    evbuffer_copyout(buffer_, buf, len);
}

void Buffer::peekAsString(size_t len, std::string* ret) const
{
    ret->resize(len);
    peekAsBytes(&*ret->begin(), ret->size());
}

void Buffer::peekAllAsString(std::string* ret) const
{
    return peekAsString(length(), ret);
}

} // namespace net
