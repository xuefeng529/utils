#include "base/FileUtil.h"
#include "base/Logging.h" // strerror_tl

#include <boost/static_assert.hpp>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

using namespace base;

bool FileUtil::fileExists(const std::string& filename)
{
	struct stat st;
	return stat(filename.c_str(), &st) == 0;
}

bool FileUtil::isDirectory(const std::string& filename)
{
	struct stat st;
	if (stat(filename.c_str(), &st) == -1)
	{
		return false;
	}

	return S_ISDIR(st.st_mode);
}

bool FileUtil::isFile(const std::string& filename)
{
	struct stat st;
	if (stat(filename.c_str(), &st) == -1)
	{
		return false;
	}

	return S_ISREG(st.st_mode);
}

bool FileUtil::getFileContents(const std::string& filename, std::string* content)
{
	char buf[8192];
	FILE *fp = fopen(filename.c_str(), "rb");
	if (!fp)
	{
		return false;
	}

	while (!feof(fp) && !ferror(fp))
	{
		size_t n = fread(buf, 1, sizeof(buf), fp);
		if (n > 0)
		{
			content->append(buf, n);
		}
	}

	if (ferror(fp))
	{
		fclose(fp);
		return false;
	}
	
	fclose(fp);
	return true;
}

bool FileUtil::putFileContents(const std::string& filename, const std::string& content)
{
	FILE *fp = fopen(filename.c_str(), "wb");
	if (!fp)
	{
		return false;
	}

	size_t n = fwrite(content.data(), 1, content.size(), fp);
	fclose(fp);
	return (n == content.size() ? true : false);
}

FileUtil::AppendFile::AppendFile(const char* filename)
  : fp_(::fopen(filename, "ae")),  // 'e' for O_CLOEXEC
    writtenBytes_(0)
{
  assert(fp_);
  ::setbuffer(fp_, buffer_, sizeof buffer_);
  // posix_fadvise POSIX_FADV_DONTNEED ?
}

FileUtil::AppendFile::~AppendFile()
{
  ::fclose(fp_);
}

void FileUtil::AppendFile::append(const char* logline, const size_t len)
{
  size_t n = write(logline, len);
  size_t remain = len - n;
  while (remain > 0)
  {
    size_t x = write(logline + n, remain);
    if (x == 0)
    {
      int err = ferror(fp_);
      if (err)
      {
        fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
      }
      break;
    }
    n += x;
    remain = len - n; // remain -= x
  }

  writtenBytes_ += len;
}

void FileUtil::AppendFile::flush()
{
  ::fflush(fp_);
}

size_t FileUtil::AppendFile::write(const char* logline, size_t len)
{
  // #undef fwrite_unlocked
  return ::fwrite_unlocked(logline, 1, len, fp_);
}

FileUtil::ReadSmallFile::ReadSmallFile(const char* filename)
  : fd_(::open(filename, O_RDONLY | O_CLOEXEC)),
    err_(0)
{
  buf_[0] = '\0';
  if (fd_ < 0)
  {
    err_ = errno;
  }
}

FileUtil::ReadSmallFile::~ReadSmallFile()
{
  if (fd_ >= 0)
  {
    ::close(fd_); // FIXME: check EINTR
  }
}

// return errno
template<typename String>
int FileUtil::ReadSmallFile::readToString(int maxSize,
                                          String* content,
                                          int64_t* fileSize,
                                          int64_t* modifyTime,
                                          int64_t* createTime)
{
  BOOST_STATIC_ASSERT(sizeof(off_t) == 8);
  assert(content != NULL);
  int err = err_;
  if (fd_ >= 0)
  {
    content->clear();

    if (fileSize)
    {
      struct stat statbuf;
      if (::fstat(fd_, &statbuf) == 0)
      {
        if (S_ISREG(statbuf.st_mode))
        {
          *fileSize = statbuf.st_size;
          content->reserve(static_cast<int>(std::min(static_cast<int64_t>(maxSize), *fileSize)));
        }
        else if (S_ISDIR(statbuf.st_mode))
        {
          err = EISDIR;
        }
        if (modifyTime)
        {
          *modifyTime = statbuf.st_mtime;
        }
        if (createTime)
        {
          *createTime = statbuf.st_ctime;
        }
      }
      else
      {
        err = errno;
      }
    }

    while (content->size() < static_cast<size_t>(maxSize))
    {
      size_t toRead = std::min(static_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
      ssize_t n = ::read(fd_, buf_, toRead);
      if (n > 0)
      {
        content->append(buf_, n);
      }
      else
      {
        if (n < 0)
        {
          err = errno;
        }
        break;
      }
    }
  }
  return err;
}

int FileUtil::ReadSmallFile::readToBuffer(int* size)
{
  int err = err_;
  if (fd_ >= 0)
  {
    ssize_t n = ::pread(fd_, buf_, sizeof(buf_)-1, 0);
    if (n >= 0)
    {
      if (size)
      {
        *size = static_cast<int>(n);
      }
      buf_[n] = '\0';
    }
    else
    {
      err = errno;
    }
  }
  return err;
}

template int FileUtil::readFile(const char* filename,
                                int maxSize,
                                std::string* content,
                                int64_t*, int64_t*, int64_t*);

template int FileUtil::ReadSmallFile::readToString(
    int maxSize,
    std::string* content,
    int64_t*, int64_t*, int64_t*);
