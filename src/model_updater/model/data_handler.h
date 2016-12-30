/**
**/

#ifndef _MODEL_UPDATER_MODEL_DATA_HANDLER_H_
#define _MODEL_UPDATER_MODEL_DATA_HANDLER_H_
//include STD C/C++ head files
#include <string>

//include third_party_lib head files
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace model_updater
{

typedef ::google::protobuf::Message ProtoConfig;
class DataHandler
{

public:
    DataHandler();
    virtual ~DataHandler();

    virtual bool Init(const ProtoConfig& config) = 0;
    virtual void Fini();
    virtual int Run();

    int RunLoop();
    void Terminate();
protected:

private:
    volatile int m_loop;

};
} // namespace model_updater
} // namespace poseidon

#endif // _MODEL_UPDATER_MODEL_DATA_HANDLER_H_

