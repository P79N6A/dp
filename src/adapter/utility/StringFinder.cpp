#include "StringFinder.h"

#define MARK_SYMBOL "%%"
using namespace std;

void StringFinder::find(const string& destStr, vector< SFItem >& outVec)
{
    size_t pos = 0;
    while ((pos = destStr.find(MARK_SYMBOL, pos)) != string::npos)
    {
        size_t s = pos;
        size_t e = 0;
        if ((pos = destStr.find(MARK_SYMBOL, pos+2)) != string::npos)
        {
            e = (pos += 2);
        }
        else
        {
            return;
        }
        SFItem data;
        data.name.assign(destStr, s, e - s);
        data.begin = s;
        data.end = e;
        outVec.push_back(data);
    }
    return;
}
