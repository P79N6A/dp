/*
* Copyright (c) 2015 Taobao.com
* All rights reserved.
* 文件名称：DevIdDecoder.h
* 摘要：imei/idfa解密器
* 作者：jimmy.gj<jimmy.gj@taobao.com>
* 日期：2015.11.20
* 修改摘要：
* 修改者：
* 修改日期：
*/
#ifndef DEV_ID_DECODER_H
#define DEV_ID_DECODER_H

#include <string>
#include <vector>
#include <openssl/blowfish.h>
#include "md5.h"
#include "StringCodec.h"
#include "CacheMap.h"

#define KEY_VERSION 1
#define KEY_LENGTH 16

class DevIdDecoder
{
    public:
        DevIdDecoder();
        ~DevIdDecoder(){}
        void decode(const std::string& in, std::string& out,
                           const std::string &hex_key);
    private:
        std::vector<std::string> vMd5BinKey;
        char hexToBin(char c);
        char hexToBin(char ch, char cl);
        void hexStrToBin(const std::string& hex, std::string& bin);
};

#endif


