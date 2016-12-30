#include "reg_zk_data.h"

#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "func.h"
#include "shm.h"
#include "log.h"

#define MAX_DATAID 512
#define MAX_VERSION 65535

#define RESERVE_SHM_KEY 1024
#define GET_DATA_SHM_KEY(_did_, _ver_) \
    (((_did_ + RESERVE_SHM_KEY) << 16) | (_ver_))

#define STRRC(_rc_) (rc_msg[ _rc_ < 0 ? -_rc_ : rc])

namespace poseidon
{

namespace mem_sync
{

//copy from ha
bool RegZKData::wait_connect_done()
{
    bool rt = true;
    do{
        int max_try_cnt = 20;
        int i=0;
        while (1) {
            if (isConnected()) {
                break;
            }

            if(++i > max_try_cnt) {
                rt = false;
                break;
            }

            usleep(100000);//100ms
        }
    } while(0);

    return rt;
}

bool RegZKData::detect_connection()
{
    if (isConnected()) return true;
    if (connect() != 0) return false;

    if (!wait_connect_done()) {
        LOG_ERROR("reconnect zk[%s] fail", _zklist.c_str());
        return false;
    };

    return true;
}

int RegZKData::init(const std::string &zklist, int ds_port)
{
    std::string ds_host;
    util::Func::get_local_ip(ds_host);

    return init(zklist, ds_host, ds_port);
}

int RegZKData::init(const std::string &zklist, const std::string &ds_host, int ds_port)
{
    int r = 0;

    do {
        if (Zk4Cpp::init(zklist) != 0) {
            r = -1;
            break;
        }

        if (connect() != 0) {
            r = -2;
            break;
        }

        if (!wait_connect_done()) {
            r = -3;
            break;
        }

        _zklist = zklist;
        _local_host = ds_host;
        _ds_port = ds_port;

    } while(0);

    if (r != 0) {
        LOG_ERROR("init zk=%s error", zklist.c_str());
    }

    return r;
}

void *RegZKData::create_free_shm(int data_id, int &shm_key, size_t shm_size)
{
    int r, version = 1, shmk, shmid;
    char data[1024], path[1024];
    uint32_t data_len = sizeof(data);
    struct Stat stat;
    void *shm_ptr;

    if (!detect_connection()) return NULL;

    snprintf(path, sizeof(path), "/mem_sync/%d/config/version", data_id);
    r = getNodeData(path, data, data_len, stat, 0);
    if (r == -1) {
        LOG_ERROR("get version, data_id=%d", data_id);
        return NULL;
    }

    if (r == 1) {
        version = util::Func::to_int(data) + 1;
    } else {
        version = 1;
    }

    do {
        version %= MAX_VERSION;
        shmk = GET_DATA_SHM_KEY(data_id, version);
        shmid = shmget(shmk, shm_size, IPC_CREAT | SHM_R | SHM_W);
        if (shmid != -1) break;

        LOG_ERROR("create shm_key=0x%x, %s, try another key", shmk, strerror(errno));
        ++version;
    } while(1);

    shm_ptr = shmat(shmid, 0, 0);
    if (shm_ptr == ((void *)-1)) {
        LOG_ERROR("attach shm_key=0x%x, %s", shmk, strerror(errno));
        shmctl(shmid, IPC_RMID, NULL);
        return NULL;
    }

    shm_key = shmk;
    return shm_ptr;
}

void *RegZKData::get_new_shm(int data_id, int &shm_key, size_t shm_size)
{
    if (data_id <= 0 || data_id >= MAX_DATAID) {
        LOG_ERROR("data_id=%d out of bound[1~%d)", data_id, MAX_DATAID);
        return NULL;
    }

    void *shm_ptr = create_free_shm(data_id, shm_key, shm_size);
    if (shm_ptr == NULL) {
        LOG_ERROR("create shm, did=%d, shm_size=%zd", data_id, shm_size);
    }

    return shm_ptr;
}

int RegZKData::reg_node(struct RegZKData::ZKNodeData &node_data, 
                        char *node_name, char *node_val)
{
    char path[1024];

    snprintf(path, sizeof(path), "/mem_sync/%d/%d/%s", 
            node_data.data_id, node_data.version, node_name);
    createNode(path, NULL, 0, NULL, node_data.flags, true);
    int r = setNodeData(path, node_val, strlen(node_val));
    if (r != 1) {
        LOG_ERROR("reg zk node=%s", path);
        return -1; 
    }   

    return 0;
}

int RegZKData::reg_data_internal(struct RegZKData::ZKNodeData &node_data)
{
    int r, version = 0, flags = 0;
    char data[1024], path[1024];
    uint32_t data_len = sizeof(data);
    struct Stat stat;

    std::string check_sum;
    if (!util::Func::md5sum((char *)node_data.shm_ptr, node_data.shm_size, check_sum)) {
        LOG_ERROR("md5sum, shum_size=%d", node_data.shm_size);
        return -1; 
    }   

    if (!detect_connection()) return -1;

    snprintf(path, sizeof(path), "/mem_sync/%d/config/version", node_data.data_id);
    r = createNode(path, "0", 1, NULL, flags, true);
    r = getNodeData(path, data, data_len, stat, 0);
    if (r != 1) {
        LOG_ERROR("get version, data_id=%d", node_data.data_id);
        return -1;
    }

    version = util::Func::to_int(data) + 1;
    node_data.version = version;

    LOG_INFO("going reg zk: data_id=%d(0x%x),"
             "version=%d(0x%x), "
             "shm_key=%d(0x%x), "
             "shm_size=%d", 
             node_data.data_id, node_data.data_id, 
             node_data.version, node_data.version, 
             node_data.shm_key, node_data.shm_key, 
             node_data.shm_size);

    snprintf(data, sizeof(data), "%d", node_data.shm_key);
    r = reg_node(node_data, (char *)"shm_key", data);
    if (r != 0) return -1; 

    snprintf(data, sizeof(data), "%zd", node_data.shm_size);
    r = reg_node(node_data, (char *)"shm_size", data);
    if (r != 0) return -1; 

    snprintf(data, sizeof(data), "%s", check_sum.c_str());
    r = reg_node(node_data, (char *)"check_sum", data);
    if (r != 0) return -1; 

    snprintf(data, sizeof(data), "%s:%d", _local_host.c_str(), _ds_port);
    r = reg_node(node_data, (char *)"data_server_addr", data);
    if (r != 0) return -1; 

    snprintf(path, sizeof(path), "/mem_sync/%d/config/version", node_data.data_id);
    snprintf(data, sizeof(data), "%d", node_data.version);
    r = setNodeData(path, data, strlen(data));
    if (r != 1) {
        LOG_ERROR("reg zk node=%s", path);
        return -1; 
    }   

    return 0;
}

int RegZKData::detach_old_shm(int data_id)
{
    using std::map;

    map<int, map<int, struct ZKNodeData> >::iterator 
    iter = _shm_map.find(data_id);
    if (iter == _shm_map.end()) return 0;

    map<int, struct ZKNodeData> &ver_map = iter->second;

    do {
        if (ver_map.size() <= 3) break;

        map<int, struct ZKNodeData>::iterator sub_iter = ver_map.begin();
        struct ZKNodeData &zkdata = sub_iter->second;

        LOG_INFO("detach old data_id=%d, ver=%d, shm_key=%d(0x%x), ver_left=%d", 
                data_id, zkdata.version, zkdata.shm_key, zkdata.shm_key, ver_map.size());
        util::shm::ShmDetach(zkdata.shm_ptr);
        ver_map.erase(sub_iter->first);
    } while(1);

    return 0;
}

int RegZKData::del_old_shm(int data_id)
{
    using namespace std;

    int r;
    char data[1024], path[1024];
    uint32_t data_len = sizeof(data);
    struct Stat stat;
    list<string> childs;

    detach_old_shm(data_id);

    snprintf(path, sizeof(path), "/mem_sync/%d", data_id);
    r = getNodeChildren(path, childs, 0);
    if (r != 1) {
        LOG_ERROR("getNodeChildren, rc=%d", r);
        return -1;
    }

    set<int> vers;
    list<string>::iterator iter;
    for (iter = childs.begin(); iter != childs.end(); ++iter) {
        if (!isdigit(iter->at(0))) continue;

        vers.insert(atoi(iter->c_str()));
    }

    //reserve 3 versions
    if (vers.size() <= 3) return 0;

    int i = 0, max = vers.size() - 3;
    set<int>::iterator viter;
    for (viter = vers.begin(); i < max && viter != vers.end(); ++viter, ++i) {
        snprintf(path, sizeof(path), "/mem_sync/%d/%d/shm_key", data_id, *viter);
        r = getNodeData(path, data, data_len, stat, 0);
        if (r != 1) {
            LOG_ERROR("getNodeData %s, rc=%d", path, r);
            continue;
        }

        data[data_len] = '\0';
        util::shm::ShmDelete(atoi(data), 1);

        snprintf(path, sizeof(path), "/mem_sync/%d/%d", data_id, *viter);
        r = deleteNode(path, true);
        if (r != 1) {
            LOG_ERROR("deleteNode %s, rc=%d", path, r);
        }
    }

    return 0;
}

int RegZKData::reg_data(int data_id, int shm_key, void *shm_ptr, size_t shm_size)
{
    if (data_id <= 0 || data_id >= MAX_DATAID) {
        LOG_ERROR("data_id=%d out of bound[1~%d)", data_id, MAX_DATAID);
        return -1;
    }

    int flags = 0;

    struct ZKNodeData node_data;
    node_data.data_id  = data_id;
    node_data.shm_key  = shm_key;
    node_data.shm_size = shm_size;
    node_data.shm_ptr  = shm_ptr;
    node_data.flags    = flags;

    int r = reg_data_internal(node_data);
    if (r != 0) return -1;

    _shm_map[data_id][node_data.version] = node_data;
    del_old_shm(data_id);

    return node_data.version;
}

int RegZKData::reg_data(int data_id, int shm_key, size_t shm_size) {
    void * shm_ptr = util::shm::ShmAttach(shm_key, shm_size);
    if (shm_ptr == NULL) {
        LOG_ERROR("attach shm_key=%d, shm_size=%d", shm_key, shm_size);
        return -1;
    }

    return reg_data(data_id, shm_key, shm_ptr, shm_size);
}


}
}
