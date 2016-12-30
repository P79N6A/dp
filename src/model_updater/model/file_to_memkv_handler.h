/**
**/

#ifndef _MODEL_UPDATER_MODEL_FILE_TO_MEMKV_HANDLER_H_
#define _MODEL_UPDATER_MODEL_FILE_TO_MEMKV_HANDLER_H_
//include STD C/C++ head files
#include <string>
#include <stdint.h>

//include third_party_lib head files
#include "src/model_updater/model/data_handler.h"
#include "protocol/src/poseidon_proto.h"
#include "src/mem_sync/data_api/kv_reg_mgr.h"

namespace poseidon
{
namespace model_updater
{

class FileToMemkvHandler : public DataHandler
{

public:
    FileToMemkvHandler();
    virtual ~FileToMemkvHandler();
    virtual bool Init(const ProtoConfig& config);
    virtual void Fini();
    virtual int Run();

protected:
    virtual bool Update() = 0;
    bool CheckWatchFileMd5();

    bool SetMemkv(const std::string& key, const std::string val)
    {
        return poseidon::mem_sync::KVRegMgr::get_mutable_instance().put(key, val) == 0;
    }

    bool SetMemkv(const char* key, size_t key_size, const char* val, size_t val_size)
    {
        return poseidon::mem_sync::KVRegMgr::get_mutable_instance().put(key_size, key, val_size, val) == 0;
    }

protected:
    std::string m_watch_file;
    std::string m_watch_tagfile;
    uint32_t m_watch_tagfile_mtime;

    FileToMemkvConfig m_config;
};
} // namespace model_updater
} // namespace poseidon

#endif // _MODEL_UPDATER_MODEL_FILE_TO_MEMKV_HANDLER_H_

