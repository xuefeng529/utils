#include "net/Ssl.h"
#include "net/SslContext.h"
#include "net/config.h"
#include "base/Logging.h"

namespace net
{

Ssl::Ssl(const SslContext& ctx)
    : ssl_(SSL_new(ctx.get()))
{
    assert(ssl_ != NULL);
}

Ssl::~Ssl()
{
    SSL_set_shutdown(ssl_, SSL_RECEIVED_SHUTDOWN);
    SSL_shutdown(ssl_);
}

std::string Ssl::getPeerCert()
{
    std::string ret;
    X509* cert = SSL_get_peer_certificate(ssl_);
    if (cert != NULL)
    {
        char* line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        ret.append("Subject: ");
        ret.append(line);
        ret.append("\n");
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        ret.append("Issuer: ");
        ret.append(line);
        ret.append("\n");
        free(line);
        X509_free(cert);
    }
    return ret;
}

} // namespace net
