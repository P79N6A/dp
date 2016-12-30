#ifndef SRC_SCORING_SCORER_SCORER_H_
#define SRC_SCORING_SCORER_SCORER_H_

#include <map>
#include <vector>
#include "protocol/src/poseidon_proto.h"
#include "src/scoring/context/ad_context.h"
#include "src/scoring/context/query_context.h"
#include "cxr_model.h"
#include "stat_rate_model.h"

namespace poseidon {

namespace scoring {

class Scorer {
public:
    Scorer();
    virtual ~Scorer();
    virtual int Init();
    virtual int Fini();

    virtual int Prepare(QueryContext* qcontext);
    virtual int AnalyzeContext(QueryContext* qcontext);
    virtual int FillOrsMsgResponse(ScoringResponse &rsp);
    virtual bool FilterRequest(QueryContext*, AdContext*);
    virtual int FilterAdByParam(AdContext* ad_context);
    virtual int IncludeAdByParam(AdContext* ad_context);
    virtual int ScoringAds(QueryContext* qcontext, AdContext* ad_context);
    virtual int ChooseFinalAds(AdContext* ad_context, ScoringResponse & rsp);
private:
    int _seed_user_grade;
    int _seed_active_user_grade;
    int _user_grade;
    float _context_quality;
    bool _is_pdb;

    CxrModel* _cxr_model;
    StatRateModel _stat_rate_model;

    AdxParams* _adx_params;
    PosParams* _pos_params;

    std::map<int, int> _exp_int_params;
    std::map<int, float> _exp_float_params;

    bool getExpParams(int param_id, int &value) {
        std::map<int, int>::iterator it = _exp_int_params.find(param_id);
        if (it != _exp_int_params.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    bool getExpParams(int param_id, float &value) {
        std::map<int, float>::iterator it = _exp_float_params.find(param_id);
        if (it != _exp_float_params.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    virtual void calSeedUser(QueryContext* qcontext);
    virtual void calUserGrade(QueryContext* qcontext);
    virtual void calContextGrade(QueryContext* qcontext);
    bool filterByPdb(AdContext*);
    bool filterBySeedUser(QueryContext* qcontext);
    bool filterByUserGrade(QueryContext* qcontext);
    bool filterByContextGrade(QueryContext* qcontext);

};

}
}





#endif /* SRC_SCORING_SCORER_H_ */
