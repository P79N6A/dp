/**
**/

//include STD C/C++ head files
#include <math.h>

//include third_party_lib head files
#include "src/ors/scoring/bid_traffic_functor.h"
#include "util/log.h"
#include <algorithm>

namespace poseidon
{
namespace ors
{

BidTrafficFunctor::BidTrafficFunctor()
{
}

BidTrafficFunctor::~BidTrafficFunctor()
{

}

bool BidTrafficFunctor::Init()
{
    return true;
}

void BidTrafficFunctor::Fini()
{

}

bool BidTrafficFunctor::SetupContextGrade(QueryAccessor* query_accessor)
{
    m_enable_context_grade_flag = false;
    m_ranking_rate_context_grade_factor = 1.0f;
    m_context_grade_threshold = FZERO;
    m_context_grade_promote_rate_factor = 1.0f;
    m_context_grade_filter_flag = false;

    if (query_accessor->GetAdxBaseParam()->has_context_grade_param())
    {
        const ContextGradeParam& base_param = query_accessor->GetAdxBaseParam()->context_grade_param();
        m_enable_context_grade_flag = base_param.enable_context_grade_flag();
        m_context_grade_threshold = base_param.context_grade_threshold();
        m_context_grade_promote_rate_factor = base_param.context_grade_promote_rate_factor();
    }

    if (query_accessor->GetPosBaseParam()->has_context_grade_param())
    {
        const ContextGradeParam& base_param = query_accessor->GetPosBaseParam()->context_grade_param();
        m_enable_context_grade_flag = base_param.enable_context_grade_flag();
        m_context_grade_threshold = base_param.context_grade_threshold();
        m_context_grade_promote_rate_factor = base_param.context_grade_promote_rate_factor();
    }

    float context_grade = FZERO;
    if (query_accessor->IsVideo() && !query_accessor->GetTraceValue(T_ID_CONTEXT_VIDEO_GRADE_QUALITY).IsNull())
    {
        context_grade = query_accessor->GetTraceValueFloat(T_ID_CONTEXT_VIDEO_GRADE_QUALITY);
        LOG_DEBUG("context grade by video quality =  %f", context_grade);
    }
    else if (!query_accessor->GetTraceValue(T_ID_CONTEXT_SPOT_GRADE_QUALITY).IsNull()) 
    {
        context_grade = query_accessor->GetTraceValueFloat(T_ID_CONTEXT_SPOT_GRADE_QUALITY);
        LOG_DEBUG("context grade by spot quality =  %f", context_grade);
    }
    query_accessor->SetTrace(T_ID_CONTEXT_GRADE_QUALITY, context_grade);

    if (m_enable_context_grade_flag)
    {
        m_context_grade_filter_flag = true;
        m_ranking_rate_context_grade_factor = std::max(0.1f, float(pow(context_grade, m_context_grade_promote_rate_factor)));
        if (context_grade >= m_context_grade_threshold) {
            m_context_grade_filter_flag = false;
        }
    }
       
    LOG_DEBUG("enable_context_grade_flag=%d, context_grade_ranking_rate_factor=%f, context_grade=%f," 
            "context_grade_threshold=%f, context_grade_promote_rate_factor=%f, context_grade_filter_flag=%d",
            m_enable_context_grade_flag, m_ranking_rate_context_grade_factor, context_grade, 
            m_context_grade_threshold, m_context_grade_promote_rate_factor, m_context_grade_filter_flag);
    return true;
}


bool BidTrafficFunctor::SetupUserGrade(QueryAccessor* query_accessor)
{
    m_enable_user_grade_flag = false;
    m_ranking_rate_user_grade_factor = 1.0f;
    m_user_grade_threshold = 0;
    m_user_grade_promote_rate_factor = 1.0f;
    m_user_grade_plan_id = 0;
    m_user_grade_filter_flag = false;

    bool exp_flag = false;
    if (query_accessor->GetExpParam(EXP_PARAM_ORS_USER_GRADE_PLAN_ID, &m_user_grade_plan_id)
        && query_accessor->GetExpParam(EXP_PARAM_ORS_USER_GRADE_THRESHOLD, &m_user_grade_threshold)
        && query_accessor->GetExpParam(EXP_PARAM_ORS_USER_GRADE_PROMOTE_RATE_FACTOR, &m_user_grade_promote_rate_factor))
    {
        m_enable_user_grade_flag = true; 
        exp_flag = true;
    }
 
    if (!m_enable_user_grade_flag && query_accessor->GetAdxBaseParam()->has_user_grade_param())
    {
        const UserGradeParam& base_param = query_accessor->GetAdxBaseParam()->user_grade_param();
        m_enable_user_grade_flag = base_param.enable_user_grade_flag();
        m_user_grade_plan_id = base_param.user_grade_plan_id();
        m_user_grade_threshold = base_param.user_grade_threshold();
        m_user_grade_promote_rate_factor = base_param.user_grade_promote_rate_factor();
    }

    int user_grade = 0;
    if (m_user_grade_plan_id > 0 && query_accessor->GetUserTagValue(m_user_grade_plan_id, &user_grade))
    {
        query_accessor->SetTrace(T_ID_USER_GRADE, user_grade);
    }

    if (m_enable_user_grade_flag)
    {
        m_ranking_rate_user_grade_factor = 1.0f;
        m_user_grade_filter_flag = true;
        if (user_grade >= 101 && user_grade <= 105)
        {
            m_ranking_rate_user_grade_factor = exp(m_user_grade_promote_rate_factor*(105 - user_grade));
            if (user_grade <= m_user_grade_threshold) {
                m_user_grade_filter_flag = false;
            } 
        }
    }

    LOG_DEBUG("enable_user_grade_flag=%d, ranking_rate_user_grade_factor=%f, user_grade=%d," 
            "user_grade_threshold=%d, user_grade_promote_rate_factor=%f, exp_flag=%d, user_grade_filter_flag=%d",
            m_enable_user_grade_flag, m_ranking_rate_user_grade_factor, user_grade,
            m_user_grade_threshold, m_user_grade_promote_rate_factor, exp_flag, m_user_grade_filter_flag);
 
    return true;
}

bool BidTrafficFunctor::SetupSeedUserNew(QueryAccessor* query_accessor)
{
    AdxBaseParam* base_param = query_accessor->GetAdxBaseParam();
    m_enable_seed_user_promote_flag = false;
    m_ranking_rate_seed_user_factor = 1.0f;
    m_seed_user_promote_rate_factor = 1.0f;
    m_seed_user_base_promote_rate = 1.0f;
    m_seed_user_filter_flag = false;

    int user_seed_id_active = query_accessor->GetTraceValueInterger(T_ID_USER_SEED_ID_ACTIVE);
    int user_seed_id_pay = query_accessor->GetTraceValueInterger(T_ID_USER_SEED_ID_PAY);

    if (base_param->has_active_seed_user_param())
    {
        m_enable_seed_user_promote_flag = base_param->seed_user_param().enable_seed_user_promote_flag();
        m_seed_user_threshold = base_param->seed_user_param().seed_user_threshold();
        m_seed_user_promote_rate_factor = base_param->seed_user_param().seed_user_promote_rate_factor();
        m_seed_user_base_promote_rate = base_param->seed_user_param().seed_user_base_promote_rate();
        if (m_enable_seed_user_promote_flag)
        {
            m_ranking_rate_seed_user_factor = 1.0f;
            m_seed_user_filter_flag = true;
            if (user_seed_id_active >= 21 && user_seed_id_active <= 28)
            {
                m_ranking_rate_seed_user_factor = 
                    m_seed_user_base_promote_rate * exp(m_seed_user_promote_rate_factor*(28 - user_seed_id_active));
                if (user_seed_id_active <= m_seed_user_threshold) {
                    m_seed_user_filter_flag = false;
                }

                if (user_seed_id_pay >= 11 && user_seed_id_pay <= 18 )
                {
                    m_ranking_rate_seed_user_factor *= 1.5f;
                }
            }
        }
    }

    LOG_DEBUG("enable_seed_user_promote_flag=%d, ranking_rate_seed_user_factor=%f, seed_user_base_promote_rate=%f, "
            "seed_user_promote_rate_factor=%f, user_seed_id_active=%d, user_seed_id_pay=%d, seed_user_threshold=%d, "
            "m_seed_user_filter_flag=%d", 
            m_enable_seed_user_promote_flag, m_ranking_rate_seed_user_factor, 
            m_seed_user_base_promote_rate,m_seed_user_promote_rate_factor,
            user_seed_id_active, user_seed_id_pay, m_seed_user_threshold,
            m_seed_user_filter_flag);

    return true;
}


bool BidTrafficFunctor::SetupSeedUser(QueryAccessor* query_accessor)
{
    int exp_seed_user_new_flag = 0;
    if (query_accessor->GetExpParam(EXP_PARAM_ORS_USER_SEED_NEW_FALG, &exp_seed_user_new_flag)
        && exp_seed_user_new_flag == 1) {
        LOG_DEBUG("exp_seed_user_new_flag = 1");
        return SetupSeedUserNew(query_accessor);
    }

    AdxBaseParam* base_param = query_accessor->GetAdxBaseParam();
    m_enable_seed_user_promote_flag = false;
    m_ranking_rate_seed_user_factor = 1.0f;
    m_seed_user_promote_rate_factor = 1.0f;
    m_seed_user_base_promote_rate = 1.0f;
    m_seed_user_filter_flag = false;

    int user_seed_cate_active = query_accessor->GetTraceValueInterger(T_ID_USER_SEED_CATE_ACTIVE);
    int user_seed_cate_pay = query_accessor->GetTraceValueInterger(T_ID_USER_SEED_CATE_PAY);

    if (base_param->has_seed_user_param())
    {
        m_enable_seed_user_promote_flag = base_param->seed_user_param().enable_seed_user_promote_flag();
        m_seed_user_threshold = base_param->seed_user_param().seed_user_threshold();
        m_seed_user_promote_rate_factor = base_param->seed_user_param().seed_user_promote_rate_factor();
        m_seed_user_base_promote_rate = base_param->seed_user_param().seed_user_base_promote_rate();
        if (m_enable_seed_user_promote_flag)
        {
            m_ranking_rate_seed_user_factor = 1.0f;
            m_seed_user_filter_flag = true;
            if (user_seed_cate_active >= 1101 && user_seed_cate_active <= 1103)
            {
                m_ranking_rate_seed_user_factor = 
                    m_seed_user_base_promote_rate*exp(m_seed_user_promote_rate_factor*(1103-user_seed_cate_active));
                if (user_seed_cate_active <= m_seed_user_threshold) {
                    m_seed_user_filter_flag = false;
                }

                if (user_seed_cate_pay == 1001)
                {
                    m_ranking_rate_seed_user_factor *= 1.5f;
                }
            }
        }
    }

    LOG_DEBUG("enable_seed_user_promote_flag=%d, ranking_rate_seed_user_factor=%f, seed_user_base_promote_rate=%f, "
            "seed_user_promote_rate_factor=%f,seed_cate_active=%d, seed_cate_pay=%d, seed_user_threshold=%d, "
            "m_seed_user_filter_flag=%d", 
            m_enable_seed_user_promote_flag, m_ranking_rate_seed_user_factor, 
            m_seed_user_base_promote_rate,m_seed_user_promote_rate_factor,
            user_seed_cate_active, user_seed_cate_pay, m_seed_user_threshold,
            m_seed_user_filter_flag);

    return true;
}


int BidTrafficFunctor::BeginWork(QueryAccessor* query_accessor)
{
    SetupSeedUser(query_accessor);
    SetupUserGrade(query_accessor);
    SetupContextGrade(query_accessor);
    return 0;
}

int BidTrafficFunctor::Work(AdAccessor* ad_accessor, QueryAccessor* query_accessor)
{
    float ranking_rate = ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_RATE);
    float ranking_rate_bid_traffic_factor = 1.0f;

    if (m_seed_user_filter_flag && m_context_grade_filter_flag && m_user_grade_filter_flag) {
        ranking_rate_bid_traffic_factor = FZERO;
    } else {
        ranking_rate_bid_traffic_factor = m_ranking_rate_seed_user_factor * m_ranking_rate_user_grade_factor * m_ranking_rate_context_grade_factor;
    }

    ad_accessor->SetFeature(F_ID_X_AD_RANKING_RATE_CONTEXT_GRADE_FACTOR, m_ranking_rate_context_grade_factor);
    ad_accessor->SetFeature(F_ID_X_AD_RANKING_RATE_USER_GRADE_FACTOR, m_ranking_rate_user_grade_factor);
    ad_accessor->SetFeature(F_ID_X_AD_RANKING_RATE, ranking_rate * ranking_rate_bid_traffic_factor);
    
    // 土豆流量直接过滤
    if (query_accessor->GetVideoInfo()->GetVideoSiteId() == VIDEO_SITE_TUDOU)
    {
        ad_accessor->SetFeature(F_ID_X_AD_RANKING_RATE, FZERO);
        LOG_DEBUG("VideoInfo site is tudou, ranking_rate = %f", 
                ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_RATE));
    }


    LOG_DEBUG("ad_id=%d, score=%f, ranking_rate=%f, ranking_rate_seed_user_factor=%f,"
            "ranking_rate_context_grade_factor=%f, ranking_rate_user_grade_factor=%f", 
            ad_accessor->GetAdId(),
            ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_SCORE),
            ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_RATE),
            m_ranking_rate_seed_user_factor,
            ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_RATE_CONTEXT_GRADE_FACTOR),
            ad_accessor->GetFeatureValueFloat(F_ID_X_AD_RANKING_RATE_USER_GRADE_FACTOR)); 

    return 0;
}


int BidTrafficFunctor::EndWork(QueryAccessor* /*query_accessor*/)
{
    return 0;
}

} // namespace ors
} // namespace poseidon

