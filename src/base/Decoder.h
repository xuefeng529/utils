#ifndef BASE_DECODER_H
#define BASE_DECODER_H

#include <string>

#include <stdint.h>
#include <assert.h>

namespace base
{

class Decoder
{
public:
	Decoder(const char* d, size_t n) : data_(d), size_(n) {}
	Decoder(const std::string& s) : data_(s.data()), size_(s.size()) {}

	bool readInt8(int8_t* ret)
	{
		return readIntT(ret);
	}

	bool readInt16(int16_t* ret)
	{
		return readIntT(ret);
	}

	bool readInt32(int32_t* ret)
	{
		return readIntT(ret);
	}

	bool readInt64(int64_t* ret)
	{
		return readIntT(ret);
	}

	bool readUint8(uint8_t* ret)
	{
		return readIntT(ret);
	}

	bool readUint16(uint16_t* ret)
	{
		return readIntT(ret);
	}

	bool readUint32(uint32_t* ret)
	{
		return readIntT(ret);
	}

	bool readUint64(uint64_t* ret)
	{
		return readIntT(ret);
	}

	bool retrieve(size_t n)
	{
		if (size_ < n)
		{
			return false;
		}

		data_ += n;
		size_ -= n;
		return true;
	}

	bool retrieveAsBytes(char* buf, size_t n)
	{
		assert(buf != NULL);
		if (size_ < n)
		{
			return false;
		}

		::memcpy(buf, data_, n);
		data_ += n;
		size_ -= n;
		return true;
	}

	template<typename HeaderType = uint32_t>
	bool retrieveAsString(std::string* ret)
	{
		assert(ret != NULL);
		HeaderType n;
		if (!readIntT(&n) || size_ < n)
		{
			return false; 
		}
	
		ret->assign(data_, n);
		data_ += n;
		size_ -= n;
		return true;
	}

	bool retrieveAsString(size_t n, std::string* ret)
	{
		assert(ret != NULL);
		if (size_ < n)
		{
			return false;
		}

		ret->assign(data_, n);
		data_ += n;
		size_ -= n;
		return true;
	}

	std::string retrieveAllAsString()
	{
		if (size_ == 0)
		{
			return "";
		}

		std::string s(data_, size_);
		data_ += size_;
		size_ = 0;
		return s;
	}

private:
	template<typename T>
	bool readIntT(T* ret)
	{
		assert(ret != NULL);
		if (size_ < sizeof(T))
		{
			return false;
		}

		*ret = *reinterpret_cast<const T*>(data_);
		data_ += sizeof(T);
		size_ -= sizeof(T);
		return true;
	}

	const char* data_;
	size_t size_;
};

} // namespace base

#endif // BASE_DECODER_H
