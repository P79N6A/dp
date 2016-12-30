/** 
 * Copyright (c) 2014 Taobao.com
 * All rights reserved.
 * 文件名称：StringSplitTools.h
 * 摘要：
 * 作者：jimmy.gj<jimmy.gj@taobao.com>
 * 日期：2014.11.16
 * 修改摘要：
 * 修改者：
 * 修改日期：
 */
#ifndef STRING_SPLIT_TOOLS_H
#define STRING_SPLIT_TOOLS_H

#include <string>
#include <utility>
#include <vector>

#define WHITESPACE_STRING " \t\v\r\f\n"

class StringSplitTools
{
    public:

        static void trimString(
                const std::string& input,
                const std::string& trim_chars,
                std::string& output);

        static void splitString(
                const std::string& str,
                char delim,
                std::vector<std::string>& r,
                bool trim_whitespace = true);
        
        static bool splitStringIntoKeyValues(
                const std::string& line,
                char key_value_delimiter,
                std::string& key,
                std::vector<std::string>& values);

        static bool splitStringIntoKeyValuePairs(
                const std::string& line,
                char key_value_delimiter,
                char key_value_pair_delimiter,
                std::vector<std::pair<std::string, std::string> >& kv_pairs);
        
};


#endif  // BASE_STRING_SPLIT_H
