
#ifndef SRC_MODEL_UPDATER_MODEL_SCORING_PARAM_HANDLER_H_
#define SRC_MODEL_UPDATER_MODEL_SCORING_PARAM_HANDLER_H_

#include "src/model_updater/model/file_to_memkv_handler.h"

namespace poseidon {
namespace model_updater {

class ScoringParamHandler: public FileToMemkvHandler {

public:
    ScoringParamHandler();
    virtual ~ScoringParamHandler();

protected:
    virtual bool Update();
};

}
}



#endif /* SRC_MODEL_UPDATER_MODEL_SCORING_PARAM_HANDLER_H_ */
