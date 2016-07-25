#include "net/Buffer.h"
#include "net/http/HttpContext.h"

#include <algorithm>

namespace net
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

bool HttpContext::parseRequest(Buffer* buf)
{
	bool ok = true;
	bool hasMore = true;
	std::string line;
	while (hasMore)
	{
		if (state_ == kExpectRequestLine)
		{
			if (buf->readLine(&line))
			{
				ok = processRequestLine(line.data(), line.data() + line.length());
				if (ok)
				{
					state_ = kExpectHeaders;
				}
				else
				{
					hasMore = false;
				}
			}
			else
			{
				hasMore = false;
			}
		}
		else if (state_ == kExpectHeaders)
		{
			if (buf->readLine(&line))
			{
				const char* colon = std::find(line.data(), line.data() + line.length(), ':');
				if (colon != line.data() + line.length())
				{
					request_.addHeader(line.data(), colon, line.data() + line.length());
				}
				else
				{
					state_ = kGotAll;
					hasMore = false;
				}
			}
			else
			{
				hasMore = false;
			}
		}
		else if (state_ == kExpectBody)
		{
			// FIXME:
		}
	}
	return ok;
}

} // namespace net
