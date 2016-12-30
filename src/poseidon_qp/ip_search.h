/**
 **/

#ifndef  _UTIL_IP_SEARCH_H_ 
#define  _UTIL_IP_SEARCH_H_

#include "qp_inc.h"

namespace poseidon {
namespace qp {
struct IpInfo {
    uint32_t start_ip;
    uint32_t end_ip;
    uint32_t city_code;
    uint32_t carrier_code;
};
struct IpIndexInfo {
    uint32_t start_ip;
    uint32_t end_ip;
    uint32_t ip_info_pos;
};

typedef boost::shared_ptr<std::map<uint32_t, IpIndexInfo> > LoadIndexGroup;

class IpSearch: public boost::serialization::singleton<IpSearch> {
public:
    IpSearch() {
        _ip_index_num = 0;
        _is_init = false;
        _copying_index = false;
    }
    bool init(const std::string &path);
    int search(const string &ip_str, uint32_t &city_code,
            uint32_t &carrier_code);
    int search(uint32_t ip, uint32_t &city_code, uint32_t &carrier_code);
    uint32_t get_index_num() {
        return _ip_index_num;
    }

private:
    boost::atomic<bool> _is_init;
    std::string _file_path;
    std::vector<IpInfo> load_ip_info_vec;
    std::vector<IpInfo> _ip_info_vec;

    boost::unordered_map<uint32_t, LoadIndexGroup> load_ip_info_index;
    boost::unordered_map<uint32_t, std::vector<IpIndexInfo> > _ip_info_index;

    uint32_t _ip_index_num;
    util::Mutex _search_mutex;
    util::Mutex _init_mutex;
    std::string _last_load_file_md5;
    bool _copying_index;

private:
    void process();
    int load_file();
    bool is_update_index();
    void gen_load_index(const IpInfo & ip_info);
    void copy_load_index();
    int group_half_search(uint32_t ip, uint32_t start, uint32_t end,
            const std::vector<IpIndexInfo> & group, uint32_t deep,
            uint32_t &info_pos);
};
}
}

#endif
