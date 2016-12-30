/**
 **/


#ifndef  _MEM_SYNC_COMMON_COMM_MACRO_H_ 
#define  _MEM_SYNC_COMMON_COMM_MACRO_H_
#include "proto.h"

namespace poseidon
{
namespace mem_sync
{

enum COMM
{
    MAX_DATAID=512,
    CHECK_SUM_MAX=256,
    IP_LEN=32,
    MS_USED_FLAG=0x234,
    IPC_MQ_KEY=0x235,

    MQ_TYPE_API=15,     //API-->AGENT
    MQ_TYPE_UPDATE=16,  //AGENT内部通讯

    SHM_MAN_KEY=0x236,
    /*key:0x10000~0x10000+1000预留给内存同步用*/
    SHM_KEY_START=0x10000,
    SHM_MAX_NUM=1000,
};

}
}



#endif   // ----- #ifndef _MEM_SYNC_COMMON_COMM_MACRO_H_  ----- 
