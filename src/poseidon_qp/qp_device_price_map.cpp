#include "qp_device_price_map.h"

DECLARE_string(tag_redis_host);
DECLARE_int32(tag_redis_port);
DECLARE_bool(tag_redis_cluster);
DEFINE_int32(price_map_load_interval, 7200, "设备价格表加载间隔,单位:秒");
namespace poseidon {
namespace qp {

void price_map_load_cb(evutil_socket_t fd, short event, void *arg) {
    util::Closure *timeout_process = (util::Closure *) arg;
    timeout_process->Run();
}

void DevicePriceMap::start_up() {
    _first_load_succ = false;
    _base = event_base_new();
    if (_base == NULL) {
        LOG_ERROR("DevicePriceMap start up error,event can not new");
        exit(-1);
    }

    util::Closure *process = util::NewCallback(this, &DevicePriceMap::process);
    util::ThreadPool::get_mutable_instance().add_task(process);

    LOG_INFO("DevicePriceMap start up...");
}

bool DevicePriceMap::get_price_tag(const string & key, DeviceData& data) {
    if (_loding)
        return false;
    bool ret = true;
    _price_map_mutex.rlock();
    boost::unordered_map<string, DeviceData>::iterator iter = _price_map.find(
            key);
    if (iter != _price_map.end()) {
        data = iter->second;
    } else
        ret = false;
    _price_map_mutex.unlock();
    return ret;
}

void DevicePriceMap::get_datas_from_redis() {

    redisAsyncContext * redis_context = _redis_client.get_redis_context(
            DEVICE_PRICE_TAG_REDIS_KEY);
    if (redis_context == NULL) {
        create_load_task();
        LOG_ERROR("device price map get redis context error");
        return;
    }
    RedisOnReply *on_reply = RedisOnReply::new_instance(
            boost::bind(&DevicePriceMap::load_redis_datas, this, _1));

    if (redisAsyncCommand(redis_context, redis_command_cb, (void *) on_reply,
            "get %s", DEVICE_PRICE_TAG_REDIS_KEY) != 0) {
        create_load_task();
        LOG_ERROR("device price map redis command error");
        return;
    }
}
void DevicePriceMap::load_redis_datas(redisReply *reply) {
    if (reply->type == REDIS_REPLY_STRING) {
        dmp::DmpDevicePriceDatas redis_datas;
        string data_md5;

        if (util::Func::md5sum(reply->str, reply->len, data_md5)) {
            if (data_md5.compare(_cur_data_md5) == 0) {
                return;
            }
        }

        if (!redis_datas.ParseFromArray(reply->str, reply->len)) {
            LOG_ERROR("device price map parse data error");
            return;
        }
        _cur_data_md5 = data_md5;
        _loding = true;
        _price_map_mutex.wlock();
        for (int i = 0; i < redis_datas.datas_size(); i++) {
            DeviceData data(new dmp::DmpDevicePriceDatas::DevicePriceData);
            data->CopyFrom(redis_datas.datas(i));
            _price_map[data->key()] = data;
        }
        _price_map_mutex.unlock();
        _loding = false;
        LOG_INFO("device price map load succ...");
        _first_load_succ = true;
    } else {
        LOG_ERROR("device price map redis reply error,type : %d", reply->type);
    }
    create_load_task();
}

void DevicePriceMap::process() {
    _redis_client.init(_base, FLAGS_tag_redis_host, FLAGS_tag_redis_port,
            FLAGS_tag_redis_cluster);

    get_datas_from_redis();

    event_base_dispatch(_base);
}

void DevicePriceMap::create_load_task() {
    if (_load_event != NULL) {
        delete _load_event;
    }
    util::Closure *load_closure = util::NewCallback(this,
            &DevicePriceMap::get_datas_from_redis);
    if (_first_load_succ) {
        _load_event = new util::TimeoutEvent(_base,
                FLAGS_price_map_load_interval, 0, price_map_load_cb,
                (void *) load_closure);
    } else {
        _load_event = new util::TimeoutEvent(_base, 0, 500 * 1000,
                price_map_load_cb, (void *) load_closure);
    }
    _load_event->active();
}

}
}
