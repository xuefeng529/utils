#ifndef CORE_CORE_OSTREAM_H__
#define CORE_CORE_OSTREAM_H__

#include "typedef.h"

namespace core
{

class  OStream
{
public:
	OStream(size_t size = 64 * 4);
	~OStream();

	void write(const char * buf, size_t size);
	char * getBuf();
	int getSize();
	void skip(int offset);

	OStream& operator << (char v);
	OStream& operator << (unsigned char v);
	OStream& operator << (signed char v);
	OStream& operator << (unsigned short v);
	OStream& operator << (short v);
	OStream& operator << (unsigned int v);
	OStream& operator << (int v);
	OStream& operator << (unsigned long v);
	OStream& operator << (long v);
	OStream& operator << (unsigned long long v);
	OStream& operator << (long long v);
	OStream& operator << (float v);
	OStream& operator << (double v);
	//OStream& operator << (const String& v);
	OStream& operator << (const std::string& v);

	/*template <typename T>
	OStream& operator << (const Vector<T>& v);*/
	template <typename T>
	OStream& operator << (const std::vector<T>& v);

	template <typename T>
	OStream& writepod(T& v);
	
	void reset()
	{
		m_cur = 0;
	}
private:
	template <typename T>
	OStream& write(T v);

	void enoughFreeSpace(size_t size);
	size_t freeSpace();
	void resize();

private:
	char*    m_buf;
	int      m_cur;
	int      m_size;
};

template <typename T>
OStream& OStream::write(T v)
{
	size_t size = sizeof(T);
	enoughFreeSpace(size);
	
	*(reinterpret_cast<T*>(&m_buf[m_cur])) = v;
	
	m_cur += static_cast<int>(size);
	return *this;
}

template <typename T>
OStream& OStream::writepod(T& v)
{
	size_t size = sizeof(T);
	enoughFreeSpace(size);

	*(reinterpret_cast<T*>(&m_buf[m_cur])) = v;

	m_cur += static_cast<int>(size);
	return *this;
}

template <typename T>
inline OStream& OStream::operator << (const std::vector<T>& v)
{
	uint32_t size = static_cast<uint32_t>(v.size());
//	assert(v.size() < 0xffff);
	write(size);
	for(uint32_t i = 0; i < size; ++i)
	{
		*this << v[i];
	}
	return *this;
}

}
#endif //CORE_CORE_OSTREAM_H__
