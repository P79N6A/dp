/**
**/

#ifndef _MODEL_UPDATER_MODEL_LR_HANDLER_H_
#define _MODEL_UPDATER_MODEL_LR_HANDLER_H_
//include STD C/C++ head files
#include <string>
#include <stdint.h>

//include third_party_lib head files
#include "src/model_updater/model/file_to_memkv_handler.h"
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace model_updater
{

class LRHandler : public FileToMemkvHandler
{

public:
    LRHandler();
    virtual ~LRHandler();

protected:
    virtual bool Update();
};
} // namespace model_updater
} // namespace poseidon

#endif // _MODEL_UPDATER_MODEL_LR_HANDLER_H_

