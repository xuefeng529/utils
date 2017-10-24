#ifndef NET_BUFFER_H
#define NET_BUFFER_H

#include "net/Endian.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

#include <assert.h>
#include <string.h>
#include <stdint.h>

struct evbuffer;

namespace net
{

class Buffer : boost::noncopyable
{
public:
	Buffer();
	Buffer(struct evbuffer* buffer);
	~Buffer();

    void append(const void* data, size_t len);
    void append(const char* data, size_t len);
	void append(const char* str);
	void append(const std::string& str);
	
	/// Append using network endian
	void appendInt64(int64_t x);
	void appendInt32(int32_t x);
	void appendInt16(int16_t x);
	void appendInt8(int8_t x);

	/// 尽量减少复制,函数调用后buffer中的数据被移走
	void removeBuffer(Buffer* buffer);
	void removeBuffer(Buffer* buffer, size_t len);

	void appendBuffer(const Buffer& buffer);

	void prepend(const void* data, size_t len);
	/// Prepend using network endian
	void prependInt64(int64_t x);
	void prependInt32(int32_t x);
	void prependInt16(int16_t x);
	void prependInt8(int8_t x);
	
	size_t length() const;

	/// Read from network endian
	int64_t readInt64();
	int32_t readInt32();
	int16_t readInt16();
	int8_t readInt8();
	bool readLine(std::string* ret);

	/// Peek from network endian
	int64_t peekInt64() const;
	int32_t peekInt32() const;
	int16_t peekInt16() const;
	int8_t peekInt8() const;
	
	/// Peek from OriginalEndian
	int64_t peekInt64WithOriginalEndian() const;
	int32_t peekInt32WithOriginalEndian() const;
	int16_t peekInt16WithOriginalEndian() const;
	
	int64_t readInt64WithOriginalEndian();
	int32_t readInt32WithOriginalEndian();
	int16_t readInt16WithOriginalEndian();

    void appendInt64WithOriginalEndian(int64_t x);
    void appendInt32WithOriginalEndian(int32_t x);
    void appendInt16WithOriginalEndian(int16_t x);

    void prependInt64WithOriginalEndian(int64_t x);
    void prependInt32WithOriginalEndian(int32_t x);
    void prependInt16WithOriginalEndian(int16_t x);

	void retrieve(size_t len);
	void retrieveInt64();
	void retrieveInt32();
	void retrieveInt16();
	void retrieveInt8();
	void retrieveAll();
    bool retrieveAsBytes(char* buf, size_t len);
    bool retrieveAsString(size_t len, std::string* ret);
	bool retrieveAllAsString(std::string* ret);
	
private:
    void peekAsBytes(char* buf, size_t len) const;	
	void peekAsString(size_t len, std::string* ret) const;
    void peekAllAsString(std::string* ret) const;

	struct evbuffer* buffer_;
	bool bufferOwner_;
};

typedef boost::shared_ptr<Buffer> BufferPtr;

} // namespace net

#endif // BASE_BUFFER_H
