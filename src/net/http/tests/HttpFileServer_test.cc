#include "net/http/HttpServer.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"
#include "net/SslContext.h"
#include "net/EventLoop.h"
#include "net/Buffer.h"
#include "base/Atomic.h"
#include "base/Timestamp.h"
#include "base/Logging.h"
#include "base/ProcessInfo.h"
#include "base/CurrentThread.h"
#include "base/FileUtil.h"
#include "base/StringUtil.h"

#include <iostream>
#include <map>
#include <boost/bind.hpp>

const size_t kBufferLimit = 32 * 1024 * 1024;

struct DownloadContext
{
	FILE* fp;
};

class HttpFileServer : boost::noncopyable
{
public:
	HttpFileServer(net::EventLoop* loop, const net::InetAddress& listenAddr)
		: httpServer_(loop, listenAddr, "http_file_server")
	{
	}

	void start(int numThreads)
	{
		httpServer_.setRequestCallback(boost::bind(&HttpFileServer::onRequest, this, _1, _2));
		httpServer_.setThreadNum(numThreads);
		httpServer_.start();
	}

private:
	void onRequest(const net::HttpRequest& request, net::HttpResponse* response)
	{
		LOG_INFO << request.methodString() << " " << request.path() << " " << request.versionString();
		std::string filename = "." + request.path();
		FILE *fp = fopen(filename.c_str(), "rb");
		if (fp == NULL)
		{
			return;
		}

		if (fseek(fp, 0, SEEK_END) == -1)
		{
			LOG_FATAL << "failed to fseek";
		}

		long fileLength = ftell(fp);
		if (fileLength == -1)
		{
			LOG_FATAL << "failed to ftell";
		}

		if (fseek(fp, 0, SEEK_SET) == -1)
		{
			LOG_FATAL << "failed to fseek";
		}

		char* buf = new char[kBufferLimit];
		size_t n = fread(buf, 1, kBufferLimit, fp);
		if (ferror(fp))
		{
			LOG_ERROR << "failed";
			response->setStatusCode(net::HttpResponse::k400BadRequest);
			response->setStatusMessage("Bad Request");
			response->setCloseConnection(false);
			fclose(fp);
			free(buf);
			return;
		}

		if (n == 0)
		{
			LOG_ERROR << "file end";
			response->setStatusCode(net::HttpResponse::k200Ok);
			response->setStatusMessage("OK");
			fclose(fp);
			free(buf);
			return;
		}
		
		LOG_INFO << "file size: " << fileLength;
		LOG_INFO << "chunk size: " << n;
		response->setBody(buf, n);
		if (n == static_cast<size_t>(fileLength))
		{
			LOG_INFO << "file end";
			response->setStatusCode(net::HttpResponse::k200Ok);
			response->setStatusMessage("OK");
			fclose(fp);
			free(buf);
			return;
		}

		LOG_INFO << "begin to chunk...";
		assert(n < static_cast<size_t>(fileLength));
		boost::shared_ptr<DownloadContext> ctx(new DownloadContext);
		ctx->fp = fp;
		
		response->setStatusCode(net::HttpResponse::k200Ok);
		response->setStatusMessage("OK");
		response->addHeader("Content-Length", base::StringUtil::int64ToStr(fileLength));
		response->enableChunked(boost::bind(&HttpFileServer::onChunked, this, ctx, _1, _2));
		free(buf);
	}

	bool onChunked(const boost::shared_ptr<DownloadContext>& ctx, net::Buffer* buf, bool* endChunked)
	{
		LOG_INFO << "chunking...";
		char* tmpBuf = new char[kBufferLimit];
		size_t n = fread(tmpBuf, 1, kBufferLimit, ctx->fp);
		if (ferror(ctx->fp))
		{
			LOG_ERROR << "failed";
			*endChunked = true;
			fclose(ctx->fp);
			free(tmpBuf);
			return false;
		}

		assert(n > 0);
		LOG_INFO << "chunk size: " << n;
		buf->append(reinterpret_cast<void*>(tmpBuf), n);
		if (n < kBufferLimit)
		{
			LOG_INFO << "file end";
			*endChunked = true;
			fclose(ctx->fp);
		}
		
		free(tmpBuf);
		return true;
	}

	net::HttpServer httpServer_;
};

int main(int argc, char* argv[])
{
	base::Logger::setLogLevel(base::Logger::DEBUG);
	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
	//int numThreads = static_cast<int>(base::ProcessInfo::getCpuCoresCount() * 2);
	net::EventLoop loop;
	HttpFileServer httpServer(&loop, net::InetAddress(argv[1], static_cast<uint16_t>(atoi(argv[2]))));
	httpServer.start(0);
	loop.loop();
}
