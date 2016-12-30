/**
 **/
#ifndef  SN_COMMON_MACRO_H_ 
#define  SN_COMMON_MACRO_H_

namespace poseidon
{
namespace sn
{

typedef int64_t TagType;
typedef std::vector<TagType> Conj;
typedef std::vector<TagType> Query;

enum
{
    POST_TYPE_NULL=0,
    POST_TYPE_ALL=1,
    POST_TYPE_HOUR=2,
};

enum
{
    POST_REGION_TAG_ID=1007,
    VIEW_TYPE_TARG_ID=19001,
    SOURCE_TARG_ID=19002,
    DEAL_ID_TARG_ID=19003,
};

enum {
    INDEX_DATA_ID = 2,
};

}
}

#endif   // ----- #ifndef SN_COMMON_MACRO_H_  ----- 


