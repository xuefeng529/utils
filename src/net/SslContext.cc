#include "net/SslContext.h"
#include "net/config.h"
#include "base/LogStream.h"
#include "base/Logging.h"

#include <pthread.h>
#include <sys/syscall.h>

namespace net
{

namespace
{

pthread_mutex_t* g_sslLocks;
long* g_sslLocksCount;

void lockingCallback(int mode, int type, const char* file, int line)
{
    if (mode & CRYPTO_LOCK)
    {
        pthread_mutex_lock(&(g_sslLocks[type]));
        g_sslLocksCount[type]++;
    }
    else
    {
        pthread_mutex_unlock(&(g_sslLocks[type]));
    }
}

unsigned long threadIdCallback(void)
{
    return static_cast<unsigned long>(::syscall(SYS_gettid));
}

void threadSetup()
{    
    g_sslLocks = static_cast<pthread_mutex_t*>(OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t)));
    g_sslLocksCount = static_cast<long*>(OPENSSL_malloc(CRYPTO_num_locks() * sizeof(long)));
    for (int i = 0; i < CRYPTO_num_locks(); i++)
    {
        g_sslLocksCount[i] = 0;
        pthread_mutex_init(&(g_sslLocks[i]), NULL);
    }

    CRYPTO_set_id_callback(threadIdCallback);
    CRYPTO_set_locking_callback(lockingCallback);
}

void threadCleanup()
{
    CRYPTO_set_locking_callback(NULL);
    for (int i = 0; i < CRYPTO_num_locks(); i++)
    {
        pthread_mutex_destroy(&(g_sslLocks[i]));
        LOG_INFO << g_sslLocksCount[i] << ": " << CRYPTO_get_lock_name(i);       
    }
    OPENSSL_free(g_sslLocks);
    OPENSSL_free(g_sslLocksCount);
}

}

SslContext::SslContext()
    : ctx_(NULL)
{
    threadSetup();
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
}

SslContext::~SslContext()
{
    if (ctx_ != NULL)
    {
        SSL_CTX_free(ctx_);
    }
    threadCleanup();
}

void SslContext::init(const std::string& cacertFile,
                      const std::string& certFile,
                      const std::string& keyFile,
                      const std::string& passwd)
{
    ctx_ = SSL_CTX_new(SSLv23_method());
    if (ctx_ == NULL)
    {
        LOG_FATAL << "SSL_CTX_new: " 
            << ERR_reason_error_string(ERR_get_error());
    }

    /// 加载CA的证书
    if (!cacertFile.empty() && !SSL_CTX_load_verify_locations(ctx_, cacertFile.c_str(), NULL))
    {
        LOG_FATAL << "SSL_CTX_load_verify_locations: " 
            << ERR_reason_error_string(ERR_get_error());
    }

    /// 加载自己的证书 
    if (!SSL_CTX_use_certificate_file(ctx_, certFile.c_str(), SSL_FILETYPE_PEM))
    {
        LOG_FATAL << "SSL_CTX_use_certificate_file: " 
            << ERR_reason_error_string(ERR_get_error());
    }

    /// 加载自己的私钥
    if (!passwd.empty())
    {
        BIO* key = BIO_new(BIO_s_file());
        if (key == NULL)
        {
            LOG_FATAL << "BIO_new: " << ERR_reason_error_string(ERR_get_error());
        }
        BIO_read_filename(key, keyFile.c_str());
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(key, NULL, NULL,
            const_cast<void*>(reinterpret_cast<const void*>(passwd.data())));
        if (pkey == NULL)
        {
            LOG_FATAL << "PEM_read_bio_PrivateKey: " 
                << ERR_reason_error_string(ERR_get_error());
            BIO_free(key);           
        }

        if (SSL_CTX_use_PrivateKey(ctx_, pkey) <= 0)
        {
            LOG_FATAL << "SSL_CTX_use_PrivateKey: " 
                << ERR_reason_error_string(ERR_get_error());
            BIO_free(key);
        }
        BIO_free(key);
    }
    else
    {
        if (!SSL_CTX_use_PrivateKey_file(ctx_, keyFile.c_str(), SSL_FILETYPE_PEM))
        {
            LOG_FATAL << "SSL_CTX_use_PrivateKey_file: " 
                << ERR_reason_error_string(ERR_get_error());
        }
    }

    /// 验证私钥和证书是否匹配 
    if (!SSL_CTX_check_private_key(ctx_))
    {
        LOG_FATAL << "SSL_CTX_check_private_key: " 
            << ERR_reason_error_string(ERR_get_error());
    }

    /// 验证对方证书
    if (!cacertFile.empty())
    {
        SSL_CTX_set_verify(ctx_, SSL_VERIFY_PEER, NULL);
    }
}

} // namespace net
