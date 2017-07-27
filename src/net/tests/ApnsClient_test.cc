#include "net/TcpClient.h"
#include "net/EventLoop.h"
#include "net/Endian.h"
#include "net/SslContext.h"
#include "base/Logging.h"
#include "base/Thread.h"
#include "base/StringUtil.h"
#include "base/Timestamp.h"

#include <boost/bind.hpp>

#include <time.h>
#include <stdio.h>

const char PRIORITY_SEND_IMMEDIATELY = 10;

#pragma pack(1)
struct PushIosData
{
    char cCmd;
    int iFrameLen;
};
struct PushIosItem
{
    char cItemId;
    short sItemLen;
};
#pragma pack()

typedef struct stCommonItem
{
    int         uMsgTimeIn;
    int         uIdentifier;
    std::string      strToken;
    std::string      strMsg;

    stCommonItem()
    {
        uMsgTimeIn = 0;
        uIdentifier = 0;
        strToken = "";
        strMsg = "";
    }
}stCommonItem;

int setIosItem(PushIosItem *cur, int iItemID, const char *data, int len)
{
    PushIosItem *PiosItem = (PushIosItem *)cur;
    PiosItem->cItemId = iItemID;
    PiosItem->sItemLen = net::sockets::hostToNetwork16(len);
    char *pData = (char *)(PiosItem + 1);
    memcpy(pData, data, len);
    return sizeof(PushIosItem) + len;
}

int hexStringToBinary(const char* pTextToken, const int iLen, char *pOut, const int iLenBuf)
{
    char strTemp[64] = "";
    int iIndex = 0;

    memset(pOut, 0x00, iLenBuf);
    for (int i = 0; i < iLen; i += 2)
    {
        strncpy(strTemp, pTextToken + i, 2);
        pOut[iIndex] = strtol(strTemp, (char**)NULL, 16);
        iIndex++;
        if (iIndex >= iLenBuf)
        {
            return -1;
        }
    }

    return 0;
}

int setPushBitPacket(char *pBuffer, stCommonItem* objData)
{
    if (objData == NULL) return 0;

    char strTokenBin[33] = { 0 };
    hexStringToBinary(objData->strToken.c_str(), objData->strToken.length(), strTokenBin, sizeof(strTokenBin));

    int iFrameLen = 0;
    PushIosData *pIosData = (PushIosData *)pBuffer;
    pIosData->cCmd = 2;

    char *pCurDataBuf = (char *)(pIosData + 1);
    iFrameLen += setIosItem((PushIosItem *)pCurDataBuf, 1, strTokenBin, 32);                                          //item1: token
    iFrameLen += setIosItem((PushIosItem *)(pCurDataBuf + iFrameLen), 2, objData->strMsg.c_str(), objData->strMsg.length());  //item2: payload data
    iFrameLen += setIosItem((PushIosItem *)(pCurDataBuf + iFrameLen), 3, (char *)&objData->uIdentifier, 4);           //item3: identifier
    int expired_time = net::sockets::hostToNetwork32((int)time(NULL) + 86400);
    iFrameLen += setIosItem((PushIosItem *)(pCurDataBuf + iFrameLen), 4, (char *)&expired_time, 4);                   //item4: expire time
    iFrameLen += setIosItem((PushIosItem *)(pCurDataBuf + iFrameLen), 5, (char *)&PRIORITY_SEND_IMMEDIATELY, 1);      //item5: priority

    pIosData->iFrameLen = net::sockets::hostToNetwork32(iFrameLen);
    return iFrameLen + sizeof(PushIosData);
}

#define MAX_TITLE_LEN   50
#define PUSH_BASE_LEN   50 // "{\"aps\":{", "\"alert\":", "\"badge\":", ",\"sound\":\"msg.wav\"}", "}"
#define MAX_PUSH_LEN    256
#define EXTEND_STRING   "..."

const uint32_t MSG_TYPE_AD = 0x0a;
typedef struct CommonItem
{
    int    m_iMsgTimeIn;
    uint32_t m_uMsgType;
    char   m_strTokenBin[65];
    std::string m_strTokenText;
    std::string m_strContent;
    int    m_iBadgeNum;
    int    m_identifier;

    CommonItem()
    {
        memset(m_strTokenBin, 0, sizeof(m_strTokenBin));
    }
} CommonItem;

typedef struct StockItem :CommonItem
{
    std::string m_strUID;
    std::string m_strNewsID;
    std::string m_strDateTime;
    std::string m_strType;
    std::string m_strMarket;
    std::string m_strCode;
} StockItem;

bool UTF8_validate(const unsigned char *pSrc, int iLen)
{
    int total = iLen;
    int in = 0;
    uint8_t byte1 = 0;

    int b0;
    byte1 = *pSrc;

    // UTF-8:   [0xxx xxxx]
    // Unicode: [0000 0000] [0xxx xxxx]

    if (byte1 <= 0x7f)
    {
        return true;
    }

    // UTF-8:   [110y yyyy] [10xx xxxx]
    // Unicode: [0000 0yyy] [yyxx xxxx]

    b0 = byte1 & 0x0FF;

    if ((b0 & 0xE0) == 0xC0 && (b0 & 0x1E) != 0)
    {

        int b1 = -1;

        if (++in < total)
        {
            b1 = pSrc[in] & 0x00FF;
            if ((b1 & 0xC0) != 0x80)
            {
                return false;
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    // UTF-8:   [1110 zzzz] [10yy yyyy] [10xx xxxx]
    // Unicode: [zzzz yyyy] [yyxx xxxx]
    if ((b0 & 0xF0) == 0xE0)
    {

        int b1 = -1;
        int b2 = -1;

        if (++in < total)
        {
            b1 = pSrc[in] & 0x00FF;
        }
        else
        {
            //
            //	length incorrect
            //
            return false;
        }

        if ((b1 & 0xC0) != 0x80
            || (b0 == 0xED && b1 >= 0xA0)
            || ((b0 & 0x0F) == 0 && (b1 & 0x20) == 0))
        {
            return false;
        }

        if (++in < total)
        {
            b2 = pSrc[in] & 0x00FF;
        }
        else
        {
            return false;
        }

        if ((b2 & 0xC0) != 0x80)
        {
            return false;
        }

        return true;
    }

    // UTF-8:   [1111 0uuu] [10uu zzzz] [10yy yyyy] [10xx xxxx]*
    // Unicode: [1101 10ww] [wwzz zzyy] (high surrogate)
    //          [1101 11yy] [yyxx xxxx] (low surrogate)
    //          * uuuuu = wwww + 1

    if ((b0 & 0xF8) == 0xF0)
    {
        int b1 = -1;
        int b2 = -1;
        int b3 = -1;
        int uuuuu;

        if (++in < total)
        {
            b1 = pSrc[in] & 0x00FF;
        }
        else
        {
            //
            //	length incorrect
            //
            return false;
        }

        if ((b1 & 0xC0) != 0x80
            || ((b1 & 0x30) == 0 && (b0 & 0x07) == 0))
        {
            //
            //	byte 2 of 4 invalid
            //
            return false;
        }

        if (++in < total)
        {
            b2 = pSrc[in] & 0x00FF;
        }
        else
        {
            //
            //	length incorrect
            //
            return false;
        }

        if ((b2 & 0xC0) != 0x80)
        {

            //
            //	byte 3 of 4 invalid
            //
            return false;
        }

        if (++in < total) {

            b3 = pSrc[in] & 0x00FF;

        }
        else
        {
            //
            //	length incorrect
            //
            return false;
        }

        if ((b3 & 0xC0) != 0x80)
        {
            //
            //	byte 4 of 4 invalid
            //
            return false;
        }

        // into surrogate characters
        uuuuu = ((b0 << 2) & 0x001C) | ((b1 >> 4) & 0x0003);
        if (uuuuu > 0x10)
        {
            //
            // invalid Surrogate
            //
            return false;
        }

        return true;
    }

    // error
    return false;
}

int GetUtf8chrlen(unsigned char bdata)
{
    unsigned char temp = 0x80;
    int num = 0;

    if (bdata < 0x80 || bdata == 0xff)//ascii code.(0-127)
    {
        return 1;
    }

    while (temp & bdata)
    {
        num++;
        temp = (temp >> 1);
    }

    return num;
}

void TruncateUTF8(char *str)
{
    int iLen = strlen(str);

    iLen--;

    while (iLen > 0 && !UTF8_validate((unsigned char *)str + iLen, GetUtf8chrlen(str[iLen])))
    {
        iLen--;
    }

    if (iLen >= 0)
    {
        str[iLen] = 0x00;
    }
}

std::string GenPayloadData(CommonItem* objData)
{
    char strBuf[256] = { 0 };
    char badgeBuf[3] = { 0 };
    std::string strExtend;
    std::string strContent;
    std::string payloadJsonStr;
    int iBadgeNum = objData->m_iBadgeNum;

    StockItem* tempData = (StockItem*)objData;
    snprintf(strBuf, sizeof(strBuf), ",\"%s\":\"%s\"", "ID", tempData->m_strNewsID.c_str());
    strExtend += strBuf;

    snprintf(strBuf, sizeof(strBuf), ",\"%s\":\"%s\"", "Dt", tempData->m_strDateTime.c_str());
    strExtend += strBuf;

    snprintf(strBuf, sizeof(strBuf), ",\"%s\":\"%s\"", "Tp", tempData->m_strType.c_str());
    strExtend += strBuf;

    snprintf(strBuf, sizeof(strBuf), ",\"%s\":\"%s\"", "UID", tempData->m_strUID.c_str());
    strExtend += strBuf;

    snprintf(strBuf, sizeof(strBuf), ",\"%s\":\"%s\"", "Mk", tempData->m_strMarket.c_str());
    strExtend += strBuf;

    snprintf(strBuf, sizeof(strBuf), ",\"%s\":\"%s\"", "Cd", tempData->m_strCode.c_str());
    strExtend += strBuf;

    // truncate content
    int iLen = MAX_PUSH_LEN - strExtend.length() - PUSH_BASE_LEN;
    if ((iLen >= 3) && (objData->m_strContent.length() >= static_cast<size_t>(iLen)))
    {
        snprintf(strBuf, sizeof(strBuf), "%s", objData->m_strContent.c_str());
        strBuf[iLen - strlen(EXTEND_STRING)] = 0x00;
        TruncateUTF8(strBuf);
        strcat(strBuf, EXTEND_STRING);
        strContent = strBuf;
    }
    else
    {
        strContent = objData->m_strContent;
    }

    payloadJsonStr.assign("{\"aps\":{");
    payloadJsonStr.append("\"alert\":");
    snprintf(strBuf, sizeof(strBuf) - 1, "\"%s\",", strContent.c_str());
    payloadJsonStr.append(strBuf);


    if (iBadgeNum > 99)
    {
        iBadgeNum = 99;
    }

    if (objData->m_uMsgType != MSG_TYPE_AD)
    {
        snprintf(badgeBuf, sizeof(badgeBuf) - 1, "%d", iBadgeNum);
        payloadJsonStr.append("\"badge\":");
        payloadJsonStr.append(badgeBuf);
        payloadJsonStr.append(",\"sound\":\"msg.wav\"}");
    }
    else
    {
        payloadJsonStr.append("\"sound\":\"msg.wav\"}");
    }

    payloadJsonStr.append(strExtend.c_str());
    payloadJsonStr.append("}");

    return payloadJsonStr;
}

const char* get_status_stringdesc(int iStatus)
{
    const char* pszDesc = NULL;
    switch (iStatus)
    {
    case 0:
        pszDesc = "No errors encountered";
        break;
    case 1:
        pszDesc = "Processing error";
        break;
    case 2:
        pszDesc = "Missing device token";
        break;
    case 3:
        pszDesc = "Missing topic";
        break;
    case 4:
        pszDesc = "Missing payload";
        break;
    case 5:
        pszDesc = "Invalid token size";
        break;
    case 6:
        pszDesc = "Invalid topic size";
        break;
    case 7:
        pszDesc = "Invalid payload size";
        break;
    case 8:
        pszDesc = "Invalid token";
        break;
    case 10:
        pszDesc = "Shutdown";
        break;
    default:
        pszDesc = "None unknow";
        break;
    }
    return pszDesc;
}

class ApnsClient
{
public:
    ApnsClient(net::EventLoop* loop,
        const net::InetAddress& listenAddr,
        const std::string& name,
        uint64_t heartbeat, 
        net::SslContext* sslCtx)
        : loop_(loop),
        client_(loop, listenAddr, name, heartbeat, sslCtx)
    {
        client_.setConnectionCallback(
            boost::bind(&ApnsClient::onConnection, this, _1));
        client_.setMessageCallback(
            boost::bind(&ApnsClient::onMessage, this, _1, _2));
        client_.setWriteCompleteCallback(
            boost::bind(&ApnsClient::onWriteComplete, this, _1));
        client_.setHearbeatCallback(
            boost::bind(&ApnsClient::onHeartbeat, this, _1));
        //client_.enableRetry();
    }

    void connect()
    {
        client_.connect();
    }

    void disconnect()
    {
        client_.disconnect();
    }

private:
    void onConnection(const net::TcpConnectionPtr& conn)
    {
        LOG_DEBUG << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN"); 
        if (conn->connected())
        {
            char buf[3000] = { 0 };
            stCommonItem pushItem;
            pushItem.uMsgTimeIn = static_cast<int>(time(NULL));
            pushItem.uIdentifier = static_cast<int>(time(NULL));
            pushItem.strToken = "ad5addaad34683ff24ee2291616e7f671827ac30307ef3779dde3c0550ab4ba0";
            StockItem stockItem;
            stockItem.m_strMarket = "0";
            stockItem.m_strCode = "300059";
            stockItem.m_strUID = "160116";
            stockItem.m_strNewsID = "123";
            stockItem.m_strDateTime = base::Timestamp::now().toFormattedString(false);
            stockItem.m_strMarket = "0";
            stockItem.m_strCode = "300059";
            stockItem.m_strContent = "test push apns";
            stockItem.m_iBadgeNum = 1;
            pushItem.strMsg = GenPayloadData(&stockItem);
            LOG_INFO << pushItem.strMsg;
            int len = setPushBitPacket(buf, &pushItem);
            LOG_INFO << "packet len: " << len;
            conn->send(buf, len);
        }               
    }

    void send(const net::TcpConnectionPtr& conn, const std::string& message)
    {
        conn->send(message);
    }

    void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
    {
        LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";
        std::string msg;
        buffer->retrieveAllAsString(&msg);
        int8_t command = static_cast<int8_t>(msg[0]);
        int8_t  status = static_cast<int8_t>(msg[1]);
        int32_t identifier = *reinterpret_cast<const int32_t*>(msg.data() +2);
        LOG_INFO << "read from apns is ok, [" << int(command) << "," << int(status) << "," << identifier << "," 
            << get_status_stringdesc(status) << "]";
        /* std::string hexStr;
         base::StringUtil::byteToHex(msg, &hexStr);
         LOG_INFO << hexStr;*/
    }

    void onWriteComplete(const net::TcpConnectionPtr& conn)
    {
        LOG_INFO << "onWriteComplete[" << conn->name() << "]";       
    }

    void onHeartbeat(const net::TcpConnectionPtr& conn)
    {
        LOG_INFO << "onHeartbeat";      
    }

    net::EventLoop* loop_;
    net::TcpClient client_;
};

int main(int argc, char* argv[])
{
    base::Logger::setLogLevel(base::Logger::INFO);
    LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
    if (argc < 6)
    {
        LOG_FATAL << "Usage: " << argv[0] << " ip port cert key passwd";
    }
  
    net::SslContext sslCtx;
    sslCtx.init("", argv[3], argv[4], argv[5]);
    net::EventLoop loop;
    net::InetAddress serverAddr(atoi(argv[2]));
    if (!net::InetAddress::resolve(argv[1], &serverAddr))
    {
        LOG_FATAL << "net::InetAddress::resolve";
    }
    
    ApnsClient cli(&loop, serverAddr, "ApnsClient", 0, &sslCtx);
    cli.connect();
    loop.loop();
    LOG_INFO << "done";
}