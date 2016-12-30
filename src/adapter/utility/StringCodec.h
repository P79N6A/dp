/**
 * Copyright (c) 2014 Taobao.com
 * All rights reserved.
 *
 * 文件名称：StringCodec.h
 * 摘要：
 *
 * 修改者：jimmy.gj
 * 修改日期：2015.08.03
 * 修改摘要: 增加了base64encode方法, 并添加了其单元测试
 */
#ifndef STRINGCODEC_H
#define STRINGCODEC_H


#include <string>

class StringCodec
{
    public:
        static void UrlEncode(const std::string& src, std::string& dst);
        static void UrlDecode(const std::string& src, std::string& dst);
        static bool Base64Decode(const std::string& src, std::string& dst);
        static bool Base64Encode(const std::string& src, std::string& dst);
        static bool Base64Encode(const char *src, int src_len, std::string& dst);
        static const std::string Replace(const std::string& src, const std::string& dst, char c);
        static bool Replace(std::string& input, const std::string& sub, const std::string& dst);
        static bool JsonEncode(const std::string& src, std::string& dst);
        static std::string ToLowercase(const std::string& src);
        static unsigned short CRC16(const char* buf, int len);
};


#endif

