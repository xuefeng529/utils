#ifndef NET_SSLCONTEXT_H
#define NET_SSLCONTEXT_H

#include <boost/noncopyable.hpp>
#include <string>

struct ssl_ctx_st;
typedef struct ssl_ctx_st SSL_CTX;

namespace net
{

class SslContext : boost::noncopyable
{
public:
    SslContext();
    ~SslContext();

    void init(const std::string& cacertFile,
              const std::string& certFile,
              const std::string& keyFile,
              const std::string& passwd);

    SSL_CTX* get() const
    { return ctx_; }

private:
    SSL_CTX* ctx_;
};

} // namespace net

#endif // NET_SSLCONTEXT_H
