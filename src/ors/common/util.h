/**
**/

#ifndef _ORS_UTIL_H_
#define _ORS_UTIL_H_
//include STD C/C++ head files
#include <string>
#include <iostream>
#include <locale>
#include <vector>
#include <boost/locale/encoding_utf.hpp>
#include <boost/algorithm/string.hpp>

//include third_party_lib head files

namespace poseidon
{
namespace ors
{

std::wstring utf8_to_wstring(const std::string& str)
{
    return boost::locale::conv::utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return boost::locale::conv::utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}  

std::string filter_pun(const std::string& str)
{
    std::locale loc("en_US.utf8");
    std::wstring ws = utf8_to_wstring(str);
    std::wstring result;
    for (size_t i = 0; i < ws.size(); i++)
    {
        if(!std::isalpha(ws[i], loc) && !std::isdigit(ws[i]))
        {
            if (i > 0) {
                result += utf8_to_wstring(" ");
            }
        }
        else
        {
            result += ws[i];
        }
    }

    return wstring_to_utf8(result);
}

} // namespace ors
} // namespace poseidon

#endif // _ORS_UTIL_H_

