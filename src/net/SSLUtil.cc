#include "net/SSLUtil.h"
#include "net/config.h"
#include "base/LogStream.h"
#include "base/Logging.h"

namespace net
{
namespace ssl
{

SSL_CTX* init(const std::string& cacertFile,
              const std::string& certFile,
              const std::string& keyFile,
              const std::string& passwd)
{
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    if (SSLeay() != OPENSSL_VERSION_NUMBER)
    {
        LOG_WARN << "Version mismatch for openssl: compiled with "
            << base::Fmt("%lx", static_cast<unsigned long>(OPENSSL_VERSION_NUMBER)) 
            << " but running with " << base::Fmt("%lx", static_cast<unsigned long>(SSLeay()));
    }

    SSL_CTX* ctx = SSL_CTX_new(SSLv23_method()); 
    if (ctx == NULL)
    {
        LOG_ERROR << "SSL_CTX_new: " << error();
        return NULL;
    }

    /// 加载CA的证书
    if (!cacertFile.empty() && !SSL_CTX_load_verify_locations(ctx, cacertFile.c_str(), NULL))
    {
        LOG_ERROR << "SSL_CTX_load_verify_locations: " << error();
        return NULL;
    }

    /// 加载自己的证书 
    if (!SSL_CTX_use_certificate_file(ctx, certFile.c_str(), SSL_FILETYPE_PEM))
    {
        LOG_ERROR << "SSL_CTX_use_certificate_file: " << error();
        return NULL;
    }

    /// 加载自己的私钥
    if (!passwd.empty())
    {
        BIO* key = BIO_new(BIO_s_file());
        if (key == NULL)
        {
            LOG_ERROR << "BIO_new: " << error();
            return NULL;
        }

        BIO_read_filename(key, keyFile.c_str());
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(key, NULL, NULL, 
            const_cast<void*>(reinterpret_cast<const void*>(passwd.data())));
        if (pkey == NULL)
        {
            LOG_ERROR << "PEM_read_bio_PrivateKey: " << error();
            BIO_free(key);
            return NULL;
        }

        if (SSL_CTX_use_PrivateKey(ctx, pkey) <= 0)
        {
            LOG_ERROR << "SSL_CTX_use_PrivateKey: " << error();
            BIO_free(key);
            return NULL;
        }

        BIO_free(key);
    }
    else
    {
        if (!SSL_CTX_use_PrivateKey_file(ctx, keyFile.c_str(), SSL_FILETYPE_PEM))
        {
            LOG_ERROR << "SSL_CTX_use_PrivateKey_file: " << error();
            return NULL;
        }
    }

    /// 验证私钥和证书是否匹配 
    if (!SSL_CTX_check_private_key(ctx))
    {
        LOG_ERROR << "SSL_CTX_check_private_key: " << error();
        return NULL;
    }

    /// 验证对方证书
    if (!cacertFile.empty())
    {
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    }
    
    return ctx;
}

void release(SSL_CTX* ctx)
{
    assert(ctx != NULL);
    SSL_CTX_free(ctx);
}

SSL* open(SSL_CTX* ctx)
{
    SSL* ssl = SSL_new(ctx);
    if (ssl == NULL)
    {
        LOG_ERROR << "SSL_new: " << error();
    }
    return ssl;
}

void close(SSL* ssl)
{
    assert(ssl != NULL);
    SSL_set_shutdown(ssl, SSL_RECEIVED_SHUTDOWN);
    SSL_shutdown(ssl);
}

std::string getPeerCert(SSL* ssl)
{
    std::string ret;
    X509* cert = SSL_get_peer_certificate(ssl);
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

std::string error()
{
    return ERR_reason_error_string(ERR_get_error());
}

} // namespace ssl
} // namespace net
