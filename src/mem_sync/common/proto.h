/**
 **/

#ifndef  _MEM_SYNC_COMMON_PROTO_H_ 
#define  _MEM_SYNC_COMMON_PROTO_H_

#include <stdint.h>

namespace poseidon
{
namespace mem_sync
{
//注意：为啥不用protobuf，主要是响应包会进行数据传输，这个数据可能达到几百M甚至几G，所以用pb并不适合，为啥MSRspHead不单独用PB，因为pb打包后长度的变长的，对流式数据，我并不知道接收多少数据开始进行解包。

#pragma pack(1)

/**
 * @brief           请求同步数据的请求包,所有数值型默认都是网络序
 **/
struct MSReq
{
    char tag_begin;     //must be '('
    char magic_num[2];  //must be "MS"
    int32_t dataid;     //dataid
    int32_t version;    //version
    int64_t offset;     //offset, default '0'
    int64_t size;       //size, 0--all shm
    char tag_end;       //must be ')'
};

struct DataInfo {
    int data_id;
    int version;
    int host;
    int port;
    char md5[128 + 1];
};


/**
 * @brief           请求同步数据的响应包头,包头紧接的就是数据,所有数值型默认都是网络序
 **/
struct MSRspHead
{
    char tag_begin;     //must be '('
    char magic_num[2];  //must be "MS"
    int32_t  result;    //0--success, other--error
    int32_t  dataid;    //dataid
    int32_t  version;   //version
    int64_t  size;      //data size
    char tag_end;       //must be ')'
};

#pragma pack()

}
}

#endif   // ----- #ifndef _MEM_SYNC_COMMON_PROTO_H_  ----- 

