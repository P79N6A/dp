/**
**/

//include STD C/C++ head files
#include <unistd.h>

//include third_party_lib head files
#include "src/model_updater/model/data_handler.h"

namespace poseidon
{
namespace model_updater
{
DataHandler::DataHandler()
{

}

DataHandler::~DataHandler()
{

}

void DataHandler::Fini()
{

}

int DataHandler::Run()
{
    return -1;
}


int DataHandler::RunLoop()
{
    int ret = 0;
    m_loop = 1;

    while(m_loop)
    {
        ret = this->Run();
        if(ret != 0)
        {
            usleep(1000*1000);
            continue;
            //return ret;
        }
        usleep(100*1000);
    }
    return 0;
}

void DataHandler::Terminate()
{
    m_loop = 0;
}




} // namespace model_updater
} // namespace poseidon

