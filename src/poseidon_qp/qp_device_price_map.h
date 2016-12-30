#pragma once

#include "qp_inc.h"
#include "redis_client.h"

namespace poseidon {
namespace qp {

typedef boost::shared_ptr<dmp::DmpDevicePriceDatas::DevicePriceData> DeviceData;

class DevicePriceMap: public boost::serialization::singleton<DevicePriceMap> {
public:
    DevicePriceMap() {
        _base = NULL;
        _load_event = NULL;
        _loding = false;
    }
    virtual ~DevicePriceMap() {
        if (_load_event != NULL) {
            delete _load_event;
        }
    }

    void start_up();
    bool get_price_tag(const string & key, DeviceData& data);

protected:
    boost::unordered_map<string, DeviceData> _price_map;
    util::Mutex _price_map_mutex;
    RedisClient _redis_client;
    struct event_base* _base;
    util::TimeoutEvent *_load_event;
    string _cur_data_md5;
    bool _first_load_succ;
    bool _loding;

protected:
    void get_datas_from_redis();
    void create_load_task();
    void load_redis_datas(redisReply *reply);
    void process();
};

}
}
