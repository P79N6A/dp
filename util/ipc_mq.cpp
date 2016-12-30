/**
 **/

#include "ipc_mq.h"

#include <sys/msg.h>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

namespace poseidon
{
namespace util
{
namespace ipc
{

/**
 * @brief       初始化
 * @param key   [IN], MQ的ipc key值
 * @param flag  [IN], ipc flag
 * @return      0 if success, or other
 **/
int MQ::init(int key, int flag)
{
    int rt=0;
    do{
        msqid_=msgget(key, flag);
        if(msqid_ < 0)
        {
            if(errno==ENOENT)
            {
                msqid_=msgget(key, flag|IPC_CREAT);
                if(msqid_ < 0)
                {
                    rt=-1;
                    break;
                }
            }else
            {
                rt=-2;
                break;
            }
        }
    }while(0);
    return rt;
}


/**
 * @brief           push一个消息进入消息队列
 * @param type      [IN],消息类型
 * @param data      [IN] ,消息的内容, NULL if not data
 * @param size      [IN] ,data的长度, size if not data
 * @return          0 if success, or other
 **/
int MQ::push(long type, const void * data, int size)
{
    int rt=0;
    char * msgdata = NULL;
    do{
        msgdata=new(std::nothrow) char[sizeof(long)+size];
        if(msgdata == NULL)
        {
            rt=-1;
            break;
        }
        *(long *)msgdata=type;
//        int total_size=sizeof(long);
        if(data != NULL && size > 0)
        {
//            total_size=size+sizeof(long);
            memcpy(msgdata+sizeof(long), data, size);
        }
        int flag=IPC_NOWAIT;
        rt=msgsnd(msqid_, msgdata, size, flag);
        if(rt < 0)
        {
            if(errno==EAGAIN)
            {
                rt=-2;
            }else
            {
                rt=-3;
            }
            break;
        }

    }while(0);

    if (msgdata) {
    	delete[] msgdata;
    }
    return rt;
}


/**
 * @brief           从消息队列里面取出一个消息
 * @param type      [IN],指定要取的消息类型,0-不限类型
 * @param outtype   [OUT],取出消息的类型
 * @param data      [OUT],返回的消息数据
 * @param size      [OUT],返回data的长度
 * @return          0 if success, or other
 **/
int MQ::get(long type, long & outtype, void * data, int & size, int flag)
{
    int rt=0;
    do{
        char buf[8192];
        ssize_t ndata_len=msgrcv(msqid_, buf, 8192, type, flag); 
        if(ndata_len < 0)
        {
            rt=-1;
        }
        outtype=*(long *)buf;
        size=ndata_len;
        if(size > 0)
        {
            memcpy(data, buf+sizeof(long), size);
        }

    }while(0);
    return rt;
}

}
} 
}

