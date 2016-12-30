/**
**/

#ifndef _ORS_COMMON_DEF_H_
#define _ORS_COMMON_DEF_H_
//include STD C/C++ head files


//include third_party_lib head files


namespace poseidon
{
namespace ors
{

const int MAX_AD_NUM = 20000;
const int MAX_CANDIDATE_AD_NUM = 256;
const float MIN_PRICE_THRESHOLD_INTO_CANDIDATE_AD_SET = 0.9f;
const float FZERO = 0.0f; // 浮点0值

enum AdxID
{
    ADX_ID_TANX = 1,
    ADX_ID_YOUKU = 2,
    ADX_ID_ALIGAME = 3,
    ADX_ID_CHANCE = 4, 
    ADX_ID_YUNOS = 5,
    ADX_ID_IQIYI = 6,
    ADX_ID_TOUTIAO = 7,
    ADX_ID_WEIBO = 8,
    ADX_ID_MAX_NUM = 9
};

enum BillingType
{
    CPT = 1,
    CPA = 2,
    CPC = 3,
    CPM = 4,
    CPD = 5
};

enum VideoSiteId
{
    VIDEO_SITE_YOUKU = 1,
    VIDEO_SITE_TUDOU = 2,
    VIDEO_SITE_IQIYI = 3
};

enum OsType
{
    OS_TYPE_ANDROID = 1,
    OS_TYPE_IOS = 2
};

enum AdFeatureID 
{
    F_ID_X_ADX_CTR = 0, 
    F_ID_X_ADX_CVR,
    F_ID_X_ADX_CPA,

    F_ID_X_ADX_OS_CTR,
    F_ID_X_ADX_OS_CVR,
    F_ID_X_ADX_OS_CPA,

    F_ID_X_ADX_POS_CTR,
    F_ID_X_ADX_POS_CVR,

    F_ID_X_ADX_POS_VIEWTYPE_CTR,
    F_ID_X_ADX_POS_VIEWTYPE_CVR,
    F_ID_X_ADX_POS_VIEWTYPE_CPA,

    F_ID_X_ADX_POS_CAMPAIGN_CTR,
    F_ID_X_ADX_POS_CAMPAIGN_CVR,
    F_ID_X_ADX_POS_CAMPAIGN_CPA,

    F_ID_X_ADX_POS_AD_CTR,
    F_ID_X_ADX_POS_AD_CVR,

    F_ID_X_ADX_POS_CREATIVE_CTR,
    F_ID_X_ADX_POS_CREATIVE_CVR,
    F_ID_X_ADX_POS_CREATIVE_CPM,
    F_ID_X_ADX_POS_CREATIVE_CPC,
    F_ID_X_ADX_POS_CREATIVE_CPA,

    F_ID_X_CAMPAIGN_PAY_FACTOR,
    F_ID_X_AD_PAY_FACTOR,

    F_ID_X_STAT_RATE_CTR,
    F_ID_X_STAT_RATE_CVR,

    F_ID_X_ORG_PRICE,

    F_ID_X_BUDGET_PACING_RATIO,

    F_ID_X_AD_CTR,
    F_ID_X_AD_CVR,

    F_ID_X_AD_RANKING_SCORE, // 算法打分, 用于Ranking
    F_ID_X_AD_RANKING_RATE, // 广告进入Ranking概率
    F_ID_X_AD_BID_PRICE, // 算法出价

    F_ID_X_AD_FILTER, // 广告过滤标志
    F_ID_X_AD_IS_CANDIDATE, // 是否候选集广告
    F_ID_X_AD_IS_TOPN, // 是否Topn广告

    F_ID_X_AD_UNIT_COST, // 广告单位成本

    F_ID_X_AD_BIDDING_EXPLORE_FLAG, // 出价探测标志位

    F_ID_X_AD_RANKING_SCORE_USER_GRADE_FACTOR, // 用户因子
    F_ID_X_AD_RANKING_SCORE_CONTEXT_GRADE_FACTOR, // 内容/上下文因子
    F_ID_X_AD_RANKING_SCORE_CVR_FACTOR, // cvr 因子
    F_ID_X_AD_RANKING_SCORE_PRICE_FACTOR, // 价格因子
    F_ID_X_AD_RANKING_SCORE_FREQUENCY_FACTOR, // 频次因子
    F_ID_X_AD_RANKING_SCORE_PAY_FACTOR, // 付费因子

    F_ID_X_AD_RANKING_RATE_USER_GRADE_FACTOR, // 用户因子
    F_ID_X_AD_RANKING_RATE_CONTEXT_GRADE_FACTOR, // 内容/上下文因子
    F_ID_X_AD_RANKING_RATE_CPA_FACTOR, // CPA因子
    F_ID_X_AD_RANKING_RATE_BUDGET_FACTOR, // 预算因子




    F_ID_X_BIDDING_MODE, // 出价模式
    F_ID_X_BIDDING_FIXED_PRICE, // 固定出价模式时的价格

    F_ID_X_AD_TOPN_WEIGHT, // 广告从候选集进入topn广告的权重

    F_ID_MAX_NUM = 64
};

enum QueryTraceId
{
    T_ID_USER_SEED_CATE_PAY = 0, // 种子用户类别
    T_ID_USER_SEED_CATE_ACTIVE,
    T_ID_USER_SEED_ID_PAY, // 新的种子用户ID
    T_ID_USER_SEED_ID_ACTIVE,
    T_ID_USER_GRADE,  // 用户分级值
    T_ID_CONTEXT_VIDEO_GRADE_QUALITY, // 视频内容分级质量因子
    T_ID_CONTEXT_SPOT_GRADE_QUALITY, // 广告位分级质量因子
    T_ID_CONTEXT_GRADE_QUALITY, // 内容分级质量因子
    
    T_ID_BIDDING_EXPLORE_FLAG, // 出价探测标志位

    T_ID_MAX_NUM = 16
};

enum UserTagType
{
    USER_TAG_TYPE_USER_GRADE = 5001,
    USER_TAG_TYPE_SEED_USER  = 5002
};

#define F_VALUE_NULL -1
struct FValue
{    
    union    
    {        
        float real;        
        int fixed;    
    } u;
    bool IsNull() const
    {
        return this->u.fixed == F_VALUE_NULL;
    }
    void SetNull()    
    {        
        this->u.fixed = F_VALUE_NULL;    
    }
};

#define ATTR_SVR_ORS_REQ_COUNT 44 
#define ATTR_SVR_ORS_RSP_COUNT 45 
#define ATTR_SVR_ORS_AVG_PROCESS_TIME 46 
#define ATTR_SVR_ORS_PARSE_REQ_PROTO_ERROR_COUNT 47 
#define ATTR_SVR_ORS_INNER_PROCESS_ERROR_COUNT 48 
#define ATTR_SVR_ORS_AVG_AD_NUM 49 
#define ATTR_SVR_ORS_NOT_RSP_TOPN_COUNT 50 
#define ATTR_SVR_ORS_HAS_RSP_TOPN_COUNT 51 
#define ATTR_SVR_ORS_NOT_ALGO_TOPN_COUNT 52 
#define ATTR_SVR_ORS_HAS_ALGO_TOPN_COUNT 53


#define EXP_PARAM_ORS_LR_CTR_MODEL_ID 3
#define EXP_PARAM_ORS_LR_CVR_MODEL_ID 4
#define EXP_PARAM_ORS_USER_GRADE_PLAN_ID 5
#define EXP_PARAM_ORS_USER_GRADE_THRESHOLD 6
#define EXP_PARAM_ORS_USER_GRADE_PROMOTE_RATE_FACTOR 7
#define EXP_PARAM_ORS_CREATIVE_FREQUENCY_LIMIT_CNT 11
#define EXP_PARAM_ORS_CAMPAIGN_FREQUENCY_LIMIT_CNT 12
#define EXP_PARAM_ORS_TOP_PROB_FLAG 13
#define EXP_PARAM_ORS_TOP_PROB_FACTOR 14
#define EXP_PARAM_ORS_HIGH_PRICE_MODE_ID 15
#define EXP_PRARM_ORS_PAY_FACTOR_FLAG 16
#define EXP_PARAM_ORS_USER_SEED_NEW_FALG 17

} // namespace ors
} // namespace poseidon

#endif // _ORS_COMMON_DEF_H_

