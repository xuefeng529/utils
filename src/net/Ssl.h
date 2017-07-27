#ifndef NET_SSL_H
#define NET_SSL_H

#include <boost/noncopyable.hpp>
#include <string>

struct ssl_st;
typedef struct ssl_st SSL;

namespace net
{

class SslContext;

class Ssl : boost::noncopyable
{
public:
    Ssl(const SslContext& ctx);
    ~Ssl();

    SSL* get() const
    { return ssl_; }

    std::string getPeerCert();

private:
    SSL* ssl_;
};

} // namespace net

#endif // NET_SSL_H
