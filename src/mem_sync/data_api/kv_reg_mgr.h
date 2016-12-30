#ifndef  _MEM_SYNC_DATA_API_KV_REG_MGR_H_
#define  _MEM_SYNC_DATA_API_KV_REG_MGR_H_

#include <string>
#include <boost/serialization/singleton.hpp>

namespace poseidon
{

namespace mem_sync
{
class RegZKData;
class MemKV;

class KVRegMgr: public boost::serialization::singleton<KVRegMgr>
{
    public:
        KVRegMgr();

        ~KVRegMgr();

        /**
         * @brief call this before going on other operations.
         * @ds_port dataserver port
         * @ret 0: success
         *    <>0: something error
         */
        int  init(const std::string &zklist, int ds_port);

        /**
         * @brief call this before going on other operations.
         * @ds_ip  dataserver ip
         * @ds_port dataserver port
         * @ret 0: success
         *    <>0: something error
         */
        int  init(const std::string &zklist, const std::string &ds_ip, int ds_port);

        /**
         * @ret 0: success
         *      1: overwrite, key existed
         *     <0: put fail, some error ocurred
         */
        int  put(const std::string &key, const std::string &val);

        /**
         * @ret 0: success
         *      1: overwirte, key existed
         *     <0: put fail, some error ocurred
         */
        int  put(size_t key_size, const char *key, size_t val_size, const char *val);

        /**
         * @brief Clear data been put. May call this if something error.
         */
        void reset();

        /**
         * @brief Make sure all k-v entries have been put, before calling this function.
         * @ret   >0: success, return version of data_id
         *       <=0: something error
         */
        int reg_data(int data_id);

    private:
        size_t     _cap;
        RegZKData *_regzk;
        MemKV     *_kvtb;
};

}
}

#endif
