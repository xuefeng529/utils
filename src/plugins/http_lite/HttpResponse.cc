#include "plugins/http_lite/HttpResponse.h"
#include "base/Buffer.h"

#include <stdio.h>

namespace plugin
{
namespace http
{

void HttpResponse::appendToBuffer(base::Buffer* output) const
{
	char buf[32];
	snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", statusCode_);
	output->append(buf);
	output->append(statusMessage_.c_str());
	output->append("\r\n");

	if (!body_.empty())
	{
		snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", body_.size());
		output->append(buf);
	}

	if (closeConnection_)
	{
		output->append("Connection: close\r\n");
	}
	else
	{
		output->append("Connection: Keep-Alive\r\n");
	}

	for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
		it != headers_.end();
		++it)
	{
		output->append(it->first);
		output->append(": ");
		output->append(it->second);
		output->append("\r\n");
	}

	output->append("\r\n");
	output->append(body_);
}

} // namespace http
} // namespace plugin
