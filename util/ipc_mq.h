/**
 **/

#ifndef _UTIL_MQ_H_
#define _UTIL_MQ_H_

namespace poseidon
{
namespace util
{
namespace ipc
{

class MQ
{
public:


    /**
     * @brief       鍒濆鍖�
     * @param key   [IN], MQ鐨刬pc key鍊�
     * @param flag  [IN], ipc flag
     * @return      0 if success, or other
     **/
    int init(int key, int flag);

    
    /**
     * @brief           push涓�涓秷鎭繘鍏ユ秷鎭槦鍒�
     * @param type      [IN],娑堟伅绫诲瀷
     * @param data      [IN] ,娑堟伅鐨勫唴瀹�, NULL if not data
     * @param size      [IN] ,data鐨勯暱搴�, size if not data
     * @return          0 if success, or other
     **/
    int push(long type, const void * data, int size);


    /**
     * @brief           浠庢秷鎭槦鍒楅噷闈㈠彇鍑轰竴涓秷鎭�
     * @param type      [IN],鎸囧畾瑕佸彇鐨勬秷鎭被鍨�,0-涓嶉檺绫诲瀷
     * @param outtype   [OUT],鍙栧嚭娑堟伅鐨勭被鍨�
     * @param data      [OUT],杩斿洖鐨勬秷鎭暟鎹�
     * @param size      [OUT],杩斿洖data鐨勯暱搴�
     * @return          0 if success, or other
     **/
    int get(long type, long & outtype, void * data, int & size, int flag = 0x01 /* MSG_NOERROR */);

private:
    int msqid_; //娑堟伅闃熷垪id
};

}
} 
}

#endif
