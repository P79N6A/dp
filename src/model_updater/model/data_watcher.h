/**
**/

#ifndef _MODEL_UPDATER_MODEL_DATA_WATCHER_H_
#define _MODEL_UPDATER_MODEL_DATA_WATCHER_H_
//include STD C/C++ head files
#include <vector>

//include third_party_lib head files
#include "src/model_updater/model/data_handler.h"

namespace poseidon
{
namespace model_updater
{
class DataWatcher
{

public:
    DataWatcher();
    virtual ~DataWatcher();

    virtual bool Init();
    virtual void Fini();

    virtual int Run();
    virtual void Terminate();
protected:
    volatile int m_loop;
    std::vector<DataHandler*> m_data_handlers;
    
};
} // namespace model_updater
} // namespace poseidon

#endif // _MODEL_UPDATER_MODEL_DATA_WATCHER_H_

