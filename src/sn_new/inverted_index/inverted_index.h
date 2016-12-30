/**
 **/
#ifndef _SN_INVERTED_INDEX_H_
#define _SN_INVERTED_INDEX_H_

#include <stdint.h>

#include <iostream>
#include <list>

#include "protocol/src/poseidon_proto.h"

#include "src/mem_sync/data_api/kv_api.h"

namespace poseidon {
namespace sn {

typedef int64_t TagType;
typedef std::vector<TagType> Conj;
typedef std::vector<TagType> Query;

class InvertedIndex {
public:
    enum {
        INDEX_DATA_ID = 2,
    };

    /**
     * @brief               初始化
     * @param dataid        [IN],dataid
     * @return              成功返回0, 否则返回其他
     **/
    int Init(int dataid = INDEX_DATA_ID);

    /**
     * @brief               倒排检索
     * @param query         [IN], 查询的query
     * @param ad_list       [OUT], 返回的广告列表
     * @return              成功返回0, 否则返回其他
     **/
    int QueryIndex(const Query &query, std::list<common::Ad> &ad_list);

private:
    /**
     * @brief               获取一个size所有的tag
     * @param size          [IN], 输入size
     * @param tag_set       [OUT], 返回tag集合
     * @return              成功返回0, 否则返回对应的错误码
     **/
    int GetSizeTagSet(int size, std::set<TagType> &tag_set);

    // k-->v
    // size_tag-->conj
    //    <size>_tag --> inverted_index::SizeTagValue.buffer;
    // conj-->PB
    mem_sync::KVApi kvapi_;

    int dataid_; //倒排索引数据同步的dataid
};
}
}

#endif // ----- #ifndef _SN_INVERTED_INDEX_H_  -----
