#include "ip_search.h"

DEFINE_int32(ip_group_width, 100, "IP查找标签功能，桶宽度");

namespace poseidon {
namespace qp {

bool IpSearch::init(const std::string &path) {
    if (_is_init)
        return _is_init;
    util::ScopeWMutex swMutex(&_init_mutex);
    if (_is_init)
        return _is_init;
    _file_path = path;
    util::Closure *c = util::NewCallback(this, &IpSearch::process);
    util::ThreadPool::get_mutable_instance().add_task(c);
    _is_init = true;
    return _is_init;
}

void IpSearch::process() {
    int _last_load_time = 0;
    while (true) {
        if (_last_load_time > 0)
            return;
        int now_time = time((time_t) NULL);
        if (now_time - _last_load_time > 0) {
            _last_load_time = now_time;
            if (load_file() != 0) {
                LOG_ERROR("load %s error", _file_path.c_str());
            }
        } else
            ::sleep(10);
    }
}

int IpSearch::load_file() {
    if (!is_update_index()) {
        return 0;
    }
    char line[1024] = { 0 };
    std::ifstream fin(_file_path.c_str());
    if (fin.is_open()) {
        while (fin.getline(line, sizeof(line) - 1)) {
            std::vector < std::string > columns;
            boost::split(columns, line, boost::is_any_of("\t,;"));

            IpInfo ip_info;
            if (columns.size() == 4) {
                ip_info.start_ip = strtoul(columns[0].c_str(), NULL, 10);
                ip_info.end_ip = strtoul(columns[1].c_str(), NULL, 10);
                ip_info.city_code = strtoul(columns[2].c_str(), NULL, 10);
                ip_info.carrier_code = strtoul(columns[3].c_str(), NULL, 10);
                gen_load_index(ip_info);
            }
            memset(line, 0, sizeof(line));
        }
    } else
        return -1;
    copy_load_index();
    return 0;
}

int IpSearch::search(const string &ip_str, uint32_t &city_code,
        uint32_t &carrier_code) {
    uint32_t ip = ntohl(inet_addr(ip_str.c_str()));
    return search(ip, city_code, carrier_code);
}

int IpSearch::search(uint32_t ip, uint32_t &city_code, uint32_t &carrier_code) {
    if (!_is_init)
        return -1;
    if (ip == 0)
        return -2;
    if (_copying_index)
        return -8;
    uint32_t index_key = ip / FLAGS_ip_group_width;
    util::ScopeRMutex srMutex(&_search_mutex);
    if (_ip_info_index.size() == 0)
        return -3;
    boost::unordered_map<uint32_t, std::vector<IpIndexInfo> >::iterator iter =
            _ip_info_index.find(index_key);
    if (iter == _ip_info_index.end())
        return -4;
    uint32_t info_pos = 0;
    int ret = group_half_search(ip, 0, iter->second.size() - 1, iter->second, 0,
            info_pos);
    if (ret < 0) {
        return -5;
    } else {
        if (info_pos >= _ip_info_vec.size()) {
            return -6;
        } else {
            city_code = _ip_info_vec[info_pos].city_code;
            carrier_code = _ip_info_vec[info_pos].carrier_code;
            /*std::cout<<"search_ip="<<ip<<
             ";start_ip="<<_ip_info_vec[info_pos].start_ip<<
             ";end_ip="<<_ip_info_vec[info_pos].end_ip<<
             ";city_code="<<_ip_info_vec[info_pos].city_code<<
             ";carrier_code="<<_ip_info_vec[info_pos].carrier_code<<std::endl;*/
            return 0;
        }
    }
    return -7;
}

void IpSearch::gen_load_index(const IpInfo & ip_info) {
    if (ip_info.start_ip == 0 || ip_info.end_ip == 0)
        return;
    uint32_t start = ip_info.start_ip;
    bool gen_end = false;
    while (!gen_end) {
        IpIndexInfo ip_index_info;
        ip_index_info.start_ip = start;
        uint32_t index_key = ip_index_info.start_ip / FLAGS_ip_group_width;

        LoadIndexGroup load_index_group;
        boost::unordered_map<uint32_t, LoadIndexGroup>::iterator find_iter =
                load_ip_info_index.find(index_key);
        if (find_iter == load_ip_info_index.end()) {
            LoadIndexGroup tmp(new std::map<uint32_t, IpIndexInfo>());
            load_index_group = tmp;
            load_ip_info_index[index_key] = load_index_group;
        } else {
            load_index_group = find_iter->second;
        }

        uint32_t space = start % FLAGS_ip_group_width;
        if (ip_info.end_ip - (start - space) >= FLAGS_ip_group_width) {
            start = start - space + FLAGS_ip_group_width;
            ip_index_info.end_ip = start - 1;
        } else {
            ip_index_info.end_ip = ip_info.end_ip;
            gen_end = true;
        }
        ip_index_info.ip_info_pos = load_ip_info_vec.size();

        //std::cout<<"ip_index_info start="<<ip_index_info.start_ip<<";end="<<ip_index_info.end_ip<<";pos="<<ip_index_info.ip_info_pos<<std::endl;
        //std::cout<<"load_index_group  key : "<<index_key<<";size : "<<(*load_index_group.get()).size()<<std::endl;
        //::sleep(1);

        (*load_index_group.get())[ip_index_info.start_ip] = ip_index_info;
    }
    load_ip_info_vec.push_back(ip_info);
}

void IpSearch::copy_load_index() {
    util::ScopeWMutex swMutex(&_search_mutex);
    _copying_index = true;
    _ip_index_num = 0;
    _ip_info_vec.clear();
    _ip_info_vec = load_ip_info_vec;

    _ip_info_index.clear();
    boost::unordered_map<uint32_t, LoadIndexGroup>::iterator iter1;
    for (iter1 = load_ip_info_index.begin(); iter1 != load_ip_info_index.end();
            iter1++) {
        std::vector<IpIndexInfo> search_index_group;
        LoadIndexGroup load_index_group = iter1->second;
        std::map<uint32_t, IpIndexInfo>::iterator iter2;
        for (iter2 = load_index_group->begin();
                iter2 != load_index_group->end(); iter2++) {
            search_index_group.push_back(iter2->second);
            _ip_index_num++;
        }
        _ip_info_index[iter1->first] = search_index_group;
    }
    LOG_DEBUG("_ip_info_index size : %d", _ip_info_index.size());
    load_ip_info_vec.clear();
    load_ip_info_index.clear();
    _copying_index = false;
}

int IpSearch::group_half_search(uint32_t ip, uint32_t start, uint32_t end,
        const std::vector<IpIndexInfo> & group, uint32_t deep,
        uint32_t &info_pos) {
    if (deep > 32)
        return -1;
    if (start < 0 || end < 0 || start > group.size() || end > group.size())
        return -2;
    if (end < start)
        return -3;
    uint32_t middle = (end + start) / 2;
    if (ip >= group[middle].start_ip && ip <= group[middle].end_ip) {
        info_pos = group[middle].ip_info_pos;
        return 0;
    } else {
        if (ip > group[middle].end_ip)
            start = middle + 1;
        else if (ip < group[middle].start_ip)
            end = middle - 1;
        else
            return -4;
        group_half_search(ip, start, end, group, ++deep, info_pos);
    }
}
bool IpSearch::is_update_index() {
    std::string cur_file_md5;
    int ret = util::Func::md5sum(_file_path, cur_file_md5);
    if (ret != 0) {
        LOG_ERROR("md5sum error,path : %S,ret : %d", _file_path.c_str(), ret);
        return false;
    }
    if (_last_load_file_md5.compare(cur_file_md5) == 0)
        return false;
    _last_load_file_md5 = cur_file_md5;
    return true;
}
}
}
