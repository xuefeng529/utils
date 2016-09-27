#include "plugins/openssl/Crypto.h"
#include "base/StringUtil.h"

#include <openssl/des.h>
#include <string.h>

namespace plugin
{
namespace openssl
{

static const uint8_t g_cbcIv[8] = { '0', '1', 'A', 'B', 'a', 'b', '9', '8' };

bool Crypto::desEncrypt(const std::string& plaintext, const std::string& key, std::string* ciphertext)
{
	DES_cblock keyEncrypt;
	DES_cblock ivec;
	memset(keyEncrypt, 0, 8);
	memcpy(keyEncrypt, key.c_str(), key.length());
	DES_key_schedule keySchedule;
	DES_set_key_unchecked(&keyEncrypt, &keySchedule);
	memcpy(ivec, g_cbcIv, sizeof(g_cbcIv));
	size_t len = plaintext.size() % 8 ? (plaintext.size() / 8 + 1) * 8 : plaintext.size();
	uint8_t* output = new uint8_t[len/* + 16*/];
	memset(output, 0, len);
	DES_ncbc_encrypt(reinterpret_cast<const uint8_t*>(plaintext.data()), output,
		plaintext.size()/* + 1*/, &keySchedule, &ivec, DES_ENCRYPT);
	*ciphertext = base::StringUtil::byteToHexStr(reinterpret_cast<char*>(output), len/* + 16*/);
	delete[] output;
	return true;

//	int docontinue = 1;
//	const char *data = plaintext.c_str(); /* ���� */
//	int data_len;
//	int data_rest;
//	unsigned char ch;
//	unsigned char *src = NULL; /* ���������� */
//	unsigned char *dst = NULL; /* ���ܺ������ */
//	int len;
//	unsigned char tmp[8];
//	unsigned char in[8];
//	unsigned char out[8];
//	const char *k = key.c_str(); /* ԭʼ��Կ */
//	int key_len;
//#define LEN_OF_KEY 24   
//	unsigned char ckey[LEN_OF_KEY]; /* ��������Կ */
//	unsigned char block_key[9];
//	DES_key_schedule ks, ks2, ks3;
//	/* ���첹������Կ */
//	key_len = strlen(k);
//	memcpy(ckey, k, key_len);
//	memset(ckey + key_len, 0x00, LEN_OF_KEY - key_len);
//	/* ����������������ռ估����������� */
//	data_len = strlen(data);
//	data_rest = data_len % 8;
//	len = data_len + (8 - data_rest);
//	ch = 8 - data_rest;
//	src = reinterpret_cast<unsigned char *>(malloc(len));
//	dst = reinterpret_cast<unsigned char *>(malloc(len));
//	if (NULL == src || NULL == dst)
//	{
//		docontinue = 0;
//	}
//	if (docontinue)
//	{
//		int count;
//		int i;
//		/* ���첹���ļ������� */
//		memset(src, 0, len);
//		memcpy(src, data, data_len);
//		memset(src + data_len, ch, 8 - data_rest);
//		/* ��Կ�û� */
//		memset(block_key, 0, sizeof(block_key));
//		memcpy(block_key, ckey + 0, 8);
//		DES_set_key_unchecked(reinterpret_cast<const_DES_cblock*>(block_key), &ks);
//		memcpy(block_key, ckey + 8, 8);
//		DES_set_key_unchecked(reinterpret_cast<const_DES_cblock*>(block_key), &ks2);
//		memcpy(block_key, ckey + 16, 8);
//		DES_set_key_unchecked(reinterpret_cast<const_DES_cblock*>(block_key), &ks3);
//		printf("before encrypt:\n");
//		for (i = 0; i < len; i++)
//		{
//			printf("0x%.2X ", *(src + i));
//		}
//		printf("\n");
//		/* ѭ������/���ܣ�ÿ8�ֽ�һ�� */
//		count = len / 8;
//		for (i = 0; i < count; i++)
//		{
//			memset(tmp, 0, 8);
//			memset(in, 0, 8);
//			memset(out, 0, 8);
//			memcpy(tmp, src + 8 * i, 8);
//			/* ���� */
//			//DES_ecb3_encrypt(reinterpret_cast<const_DES_cblock*>(tmp), reinterpret_cast<DES_cblock*>(in), &ks, &ks2, &ks3, DES_ENCRYPT);
//			/* ���� */
//			//DES_ecb3_encrypt((const_DES_cblock*)in, (DES_cblock*)out, &ks, &ks2, &ks3, DES_DECRYPT);
//			/* �����ܵ����ݿ��������ܺ������ */
//			//memcpy(dst + 8 * i, in, 8);
//		}
//		printf("after decrypt :\n");
//		/*for (i = 0; i < len; i++)
//		{
//			printf("0x%.2X ", *(dst + i));
//		}*/
//		printf("\n");
//	}
//	if (NULL != src)
//	{
//		free(src);
//		src = NULL;
//	}
//	if (NULL != dst)
//	{
//		free(dst);
//		dst = NULL;
//	}
//	return 0;
}

bool Crypto::desDecrypt(const std::string& ciphertext, const std::string& key, std::string* plaintext)
{
	DES_cblock keyEncrypt;
	DES_cblock ivec;
	memset(keyEncrypt, 0, 8);
	memcpy(keyEncrypt, key.c_str(), key.length());
	DES_key_schedule keySchedule;
	DES_set_key_unchecked(&keyEncrypt, &keySchedule);
	memcpy(ivec, g_cbcIv, sizeof(g_cbcIv));
	size_t len = ciphertext.size() % 8 ? (ciphertext.size() / 8 + 1) * 8 : ciphertext.size();
	uint8_t* output = new uint8_t[len];
	memset(output, 0, len);
	DES_ncbc_encrypt(reinterpret_cast<const uint8_t*>(ciphertext.data()), output,
		ciphertext.size()/* + 1*/, &keySchedule, &ivec, DES_DECRYPT);
	*plaintext = reinterpret_cast<char*>(output);
	delete[] output;
	return true;
}

} // namespace openssl
} // namespace plugin