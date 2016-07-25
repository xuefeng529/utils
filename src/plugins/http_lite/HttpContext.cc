#include "base/Buffer.h"
#include "base/StringUtil.h"
#include "base/Logging.h"
#include "plugins/http_lite/HttpContext.h"

#include <strings.h>

namespace plugin
{
namespace http
{

bool HttpContext::processRequestLine(const char* begin, const char* end)
{
	bool succeed = false;
	const char* start = begin;
	const char* space = std::find(start, end, ' ');
	if (space != end && request_.setMethod(start, space))
	{
		start = space + 1;
		space = std::find(start, end, ' ');
		if (space != end)
		{
			const char* question = std::find(start, space, '?');
			if (question != space)
			{
				request_.setPath(start, question);
				request_.setQuery(question, space);
			}
			else
			{
				request_.setPath(start, space);
			}
			start = space + 1;
			succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
			if (succeed)
			{
				if (*(end - 1) == '1')
				{
					request_.setVersion(HttpRequest::kHttp11);
				}
				else if (*(end - 1) == '0')
				{
					request_.setVersion(HttpRequest::kHttp10);
				}
				else
				{
					succeed = false;
				}
			}
		}
	}
	return succeed;
}

void HttpContext::parseRequest(const char* buf, size_t len)
{
	buffer_.append(buf, len);
	while (buffer_.readableBytes() > 0)
	{
		LOG_DEBUG << "state_: " << state_;
		if (state_ == kExpectRequestLine)
		{
			const char* crlf = buffer_.findCRLF();
			if (crlf != NULL)
			{
				if (processRequestLine(buffer_.peek(), crlf))
				{
					state_ = kExpectHeaders;
					buffer_.retrieveUntil(crlf + 2);
				}
				else
				{
					buffer_.retrieveAll();
					break;
				}
			}
			else
			{
				break;
			}
		}
		else if (state_ == kExpectHeaders)
		{
			const char* crlf = buffer_.findCRLF();
			if (crlf != NULL)
			{
				const char* colon = std::find(buffer_.peek(), crlf, ':');
				if (colon != crlf)
				{
					request_.addHeader(buffer_.peek(), colon, crlf);
				}
				else
				{
					if (!request_.getHeader("Content-Length").empty())
					{
						state_ = kExpectBody;
					}
					else
					{
						requestCallback_(request_);
						reset();
					}
				}
				buffer_.retrieveUntil(crlf + 2);
			}
			else
			{
				break;
			}
		}
		else if (state_ == kExpectBody)
		{
			size_t len = base::StringUtil::strToUInt64(request_.getHeader("Content-Length").c_str());
			if (buffer_.readableBytes() >= len)
			{
				request_.setBody(buffer_.peek(), buffer_.peek() + len);
				buffer_.retrieve(len);
				requestCallback_(request_);
				reset();
			}
			else
			{
				break;
			}
		}
	}
}

} // namespace http
} // namespace plugin
