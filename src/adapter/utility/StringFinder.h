/** 
 * Copyright (c) 2014 Taobao.com
 * All rights reserved.
 * 文件名称：StringFinder.h
 * 摘要：
 * 作者：jimmy.gj<jimmy.gj@taobao.com>
 * 日期：2015.05.20
 * 修改摘要：
 * 修改者：
 * 修改日期：
 */

#include <string>
#include <vector>
#include <utility>

#ifndef STRINGFINDER_H
#define STRINGFINDER_H

struct SFItem
{
    std::string name;
    std::size_t begin;
    std::size_t end;
};
class StringFinder
{
    public:
        static void find(const std::string& destStr, std::vector< SFItem >& outVec);
};

#endif
