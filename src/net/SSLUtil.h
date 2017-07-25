#ifndef NET_SSLUTIL_H
#define NET_SSLUTIL_H

#include <string>

struct ssl_ctx_st;
typedef struct ssl_ctx_st SSL_CTX;

struct ssl_st;
typedef struct ssl_st SSL;

namespace net
{
namespace ssl
{
    
SSL_CTX* init(const std::string& cacertFile,
              const std::string& certFile, 
              const std::string& keyFile, 
              const std::string& passwd);

void release(SSL_CTX* ctx);
SSL* open(SSL_CTX* ctx);
void close(SSL* ssl);
std::string getPeerCert(SSL* ssl);
std::string error();

} // namespace ssl
} // namespace net

#endif // NET_SSLUTIL_H
