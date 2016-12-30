#include "common.h"


namespace poseidon
{

namespace adapter
{



namespace
{
//__thread char _static_to_char_buff[32];
}
//boost::split(list, src, boost::is_any_of("`"));



void adapter_replace_all(char *src, char word_to_be_find, char word_to_be_replace)
{
    if(src == NULL) return;
    char *p = src;
    while (*p) {
        while(*p && *p != word_to_be_find) {
            ++p;
        }
        if(!*p) {
            break;
        }
        *p++ = word_to_be_replace;
    }
}

}
}
