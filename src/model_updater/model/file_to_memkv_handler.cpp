/**
**/

//include STD C/C++ head files
#include <stdio.h>
#include <fstream>
#include <set>

//include third_party_lib head files
#include "src/model_updater/model/file_to_memkv_handler.h"
#include "util/log.h"
#include "util/func.h"

namespace poseidon
{
namespace model_updater
{

static std::set<int> DATAID_SET;

FileToMemkvHandler::FileToMemkvHandler()
{
    m_watch_tagfile_mtime = 0;
}

FileToMemkvHandler::~FileToMemkvHandler()
{

}

bool FileToMemkvHandler::Init(const ProtoConfig& config)
{
    m_config.CopyFrom(config);
    m_watch_tagfile_mtime = 0;
    m_watch_file = m_config.file_path();
    m_watch_tagfile =  m_config.file_path() + ".tag";

    if (DATAID_SET.find(m_config.data_id()) != DATAID_SET.end()) {
        LOG_ERROR("Dataid conflict!");
        return false;
    }
    DATAID_SET.insert(m_config.data_id());

    LOG_INFO("Init OK!");
    return true;
}


void FileToMemkvHandler::Fini()
{
    LOG_INFO("FileToMemkvHandler Fini Finish!");
}

int FileToMemkvHandler::Run()
{
    struct stat st;
    if (stat(m_watch_tagfile.c_str(), &st) < 0)
    {
        LOG_ERROR("Stat tagfile=%s failed!", m_watch_tagfile.c_str());
        return -1;
    }

    if (st.st_mtime <= m_watch_tagfile_mtime)
    {
        LOG_DEBUG("tagfile=%s No Change", m_watch_tagfile.c_str());
        return 0;
    }

    m_watch_tagfile_mtime = st.st_mtime;
    LOG_DEBUG("Watch Changing");
    if (!this->CheckWatchFileMd5())
    {
        LOG_ERROR("CheckWatchFileMd5 Error!");
        return -2;
    }

    LOG_INFO("data_id=%d Updateing", m_config.data_id());

    poseidon::mem_sync::KVRegMgr::get_mutable_instance().reset();
    if (!this->Update())
    {
        LOG_ERROR("Upadte Failed!data_id=%d", m_config.data_id());
        return -3;
    }

    int version = poseidon::mem_sync::KVRegMgr::get_mutable_instance().reg_data(m_config.data_id());
    if (version <= 0)
    {
        LOG_ERROR("KVRegMgr reg_data Failed!data_id=%d", m_config.data_id());
        return -4;
    }

    LOG_INFO("Update Finish!data_id=%d, version=%d", m_config.data_id(), version);

    LOG_INFO("Run OK!");
    return 0;
}

bool FileToMemkvHandler::CheckWatchFileMd5()
{
    std::ifstream ifs(m_watch_tagfile.c_str());
    if (!ifs.is_open())
    {
        LOG_ERROR("Open tagfile=%s Failed!", m_watch_tagfile.c_str());
        return false;
    }

    std::string line;
    if (!getline(ifs, line))
    {
        LOG_ERROR("Get Line tagfile=%s Failed!", m_watch_tagfile.c_str());
        return false;
    }

    std::string check_md5sum = line.substr(0, line.find_first_of(" \n\t"));
    LOG_DEBUG("tagfile=%s, check_md5sum=%s", m_watch_tagfile.c_str(), check_md5sum.c_str());

    std::string md5sum;

    if (!util::Func::BinaryFileMD5(m_watch_file.c_str(), &md5sum))
    {
        LOG_ERROR("md5sum of %s Failed!", m_watch_file.c_str());
        return false;
    }

    LOG_DEBUG("file=%s, md5sum=%s", m_watch_file.c_str(), md5sum.c_str());
    if (md5sum != check_md5sum)
    {
        LOG_ERROR("md5 check error! file=%s, md5sum=%s, tagfile=%s, check_md5sum=%s",
                m_watch_file.c_str(),md5sum.c_str(), m_watch_tagfile.c_str(), check_md5sum.c_str());
        return false;
    }

    return true;

}

} // namespace model_updater
} // namespace poseidon

