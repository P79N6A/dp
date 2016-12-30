/**
 **/

#ifndef SRC_SN_SN_BUILD_INVERTEDINDEX_BUILDER_H_
#define SRC_SN_SN_BUILD_INVERTEDINDEX_BUILDER_H_

#include "json/json.h"
#include "protocol/src/poseidon_proto.h"

#include "src/mem_sync/data_api/kv_reg_mgr.h"
#include "src/sn_new/common/comm_macro.h"

namespace poseidon {
namespace sn {
class InvertedIndexBuilder
    : public boost::serialization::singleton<InvertedIndexBuilder> {
public:
    /**
     * @brief                   初始化
     * @param zk_host_list      [IN], zk列表
     * @param data_server_port  [IN], data_server_port
     * @return                  成功返回0, 否则返回其他错误码
     **/
    int Init(const std::string &zk_host_list, int data_servr_port);

    /**
     * @brief               根据文件建立倒排索引
     * @param index_file    [IN], 倒排索引文件
     * @return              成功返回0, 否则返回其他错误码
     **/
    int BuildInvertedIndex(const std::string &index_file);

private:
    /**
     * @brief               解析一行广告Json格式
     * @param buf           [IN],输入json格式的广告
     * @param size          [IN], buf的长度
     * @param ad            [OUT],返回广告
     * @param vrconj        [OUT],返回广告的Conj列表
     * @return              成功返回0, 否则返回
     **/
    int ParseLine(const char *buf, int size, common::Ad &ad,
                  std::vector<Conj> &vrconj);

    /**
     * @brief               转化一个conj to str
     **/
    std::string Conj2Str(const Conj &conj);

private:
    mem_sync::KVRegMgr kvregapi_;
    Json::Reader reader_;
};
}
}

#endif // ----- #ifndef SRC_SN_SN_BUILD_INVERTEDINDEX_BUILDER_H_  -----
