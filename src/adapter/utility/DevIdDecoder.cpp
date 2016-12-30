#include "DevIdDecoder.h"
#include "md5.h"

#include <muduo/base/Logging.h>

#define KEY_VERSION 1
#define KEY_LENGTH 16
#define VERSION_OFFSET 0
#define LENGTH_OFFSET 1
#define ENCODEID_OFFSET 2
#define CHECKSUM_LEN 4
using namespace std;



DevIdDecoder::DevIdDecoder()
{

}

inline char DevIdDecoder::hexToBin(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'z')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'Z')
    {
        return c - 'A' + 10;
    }
    return 0;
}

inline char DevIdDecoder::hexToBin(char ch, char cl)
{
    return (hexToBin(ch) << 4) | hexToBin(cl);
}

void DevIdDecoder::hexStrToBin(const std::string& hex, std::string& bin)
{
    int len = hex.length();
    bin.assign(len/2, 0);
    for (int i = 0; i < len / 2; i++)
    {
        bin[i] = hexToBin(hex[i * 2], hex[i * 2 + 1]);
    }
    return;
}



void DevIdDecoder::decode(const std::string& in, std::string& out,
                          const std::string &hex_key)
{
    if (hex_key.length() == 0)
    {
        out = in;
        LOG_ERROR << "the key pointer(m_pDspId or m_pCipherKey) is null, at DevIdDecoder, so there will be use default id value, output:" << out;
        return;
    }
    // BASE64 Decode
    string b64ed;
    StringCodec::Base64Decode(in, b64ed);
    //xxxx:decodeºó£¬¸ñÊ½Îª£ºVersion(1) Len_IDencrpt(1) IDencrpt(...) CRC(4)
    // check version
    if ((int)b64ed[VERSION_OFFSET] != KEY_VERSION)
    {
        out = in;
        LOG_ERROR << "key version is error, so there will be use default id value, output:" << out;
        return;
    }


    if(vMd5BinKey.size() == 0)//第一次请求会执行，稍微耗费点时间。后续不会进入
    {
        string binKey;
        hexStrToBin(hex_key, binKey);
        MD5 md5Obj = MD5(binKey);
        vMd5BinKey.push_back(string((const char*)md5Obj.binarydigest(), KEY_LENGTH));
        vMd5BinKey.push_back(binKey);
    }
        // valid cache time is one hour

    unsigned int devIdLen = (unsigned char)b64ed[LENGTH_OFFSET];//Len_IDencrpt
    const string& ctx = vMd5BinKey[0];
    char preOut[devIdLen+1];
    memset(preOut, 0, devIdLen+1);
    for (unsigned int i = 0; i < devIdLen; ++i)
    {
        if (i < KEY_LENGTH)
        {
            preOut[i] = b64ed[ENCODEID_OFFSET+i] ^ ctx[i];
        }
        else
        {
            preOut[i] = b64ed[ENCODEID_OFFSET+i] ^ ctx[i % KEY_LENGTH];
        }
    }
    //Len_IDencrpt(1) IDencrpt(...) CRC(4)
    //LOG_DEBUG << "pre-output is:" << preOut;
    // checksum
    string allStr(b64ed, 0, 2);
    allStr.append(preOut, devIdLen);
    allStr.append(vMd5BinKey[1]);
    MD5 lastMd5Obj = MD5(allStr);
    string ck((const char*)lastMd5Obj.binarydigest(), CHECKSUM_LEN);

    if (0 == b64ed.compare(ENCODEID_OFFSET + devIdLen, CHECKSUM_LEN, ck))
    {
        out.assign(preOut, devIdLen);
        //LOG_DEBUG << "checksum success in decoding device id, final id is:" << out;
    }
    else
    {
        out = in;
        LOG_ERROR << "checksum error in decoding device id, so final id id:" << out;
    }
    return;
}


