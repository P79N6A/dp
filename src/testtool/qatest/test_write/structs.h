/**
**/

#ifndef _MODEL_UPDATER_MODEL_API_STRUCTS_H_
#define _MODEL_UPDATER_MODEL_API_STRUCTS_H_
//include STD C/C++ head files
#include <stdint.h>
#include <string.h>
#include <stdio.h>

//include third_party_lib head files


namespace poseidon
{
namespace testtool
{
#pragma pack(push, 1)
struct StatRateKey
{
    uint32_t source;
    uint64_t pid;
    uint32_t advertiser_id;
    uint32_t campaign_id;
    uint32_t ad_id;
    uint32_t creative_id;
    StatRateKey ()
    {
        memset(this, 0, sizeof(StatRateKey));
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "source=%u,pid=%lu,advertiser_id=%u,campaign_id=%u,ad_id=%u, creative_id=%u",
                            source, pid, advertiser_id, campaign_id, ad_id, creative_id);
        return str;
    }
};

struct StatRateValue
{
    int imprs;
    int clicks;
    int binds;
    float cpm;
    float cpc;
    float cpa;
    float ctr;
    float cvr;

    StatRateValue()
    {
        memset(this, 0, sizeof(StatRateValue));
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "imprs=%d, clicks=%d, binds=%d, cpm=%.6f, cpc=%.6f, cpa=%.6f, ctr=%.6f, cvr=%.6f",
                imprs, clicks, binds, cpm, cpc, cpa, ctr, cvr);
        return str;
    }
};

struct LRKey
{
    uint64_t fea_hash;
    LRKey()
    {
        fea_hash = 0;
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "fea_hash=%lu", fea_hash);
        return str;
    }
};

struct LRValue
{
    float weight;
    LRValue()
    {
        weight = 0.0f;
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "weight=%f", weight);
        return str;
    }
};

struct BudgetPacingKey
{
    uint32_t source;
    uint32_t campaign_id;
    BudgetPacingKey()
    {
        memset(this, 0, sizeof(BudgetPacingKey));
    }

    bool operator==(const BudgetPacingKey& other)
    {
        return memcmp(this, &other, sizeof(BudgetPacingKey)) == 0;
    }
    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "source=%u, campaign_id=%u", source, campaign_id);
        return str;
    }
};

struct BudgetPacingValue
{
    float budget_exceeding_ratio;
    float budget_pacing_ratio;
    int bid_mode;
    float fixed_price;

    BudgetPacingValue()
    {
        budget_exceeding_ratio = 1.0f;
        budget_pacing_ratio = 1.0f;
        fixed_price = 0.01f;
    }
    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "budget_exceeding_ratio=%.6f, budget_pacing_ratio=%.6f, bid_mode=%d, fixed_price=%f",
                budget_exceeding_ratio, budget_pacing_ratio, bid_mode, fixed_price);
        return str;
    }
};


struct BaseParamKey
{
    uint32_t source;
    uint64_t pid;
    BaseParamKey()
    {
        memset(this, 0, sizeof(BaseParamKey));
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "source=%u, pid=%lu", source, pid);
        return str;
    }
};

struct ScoringParamKey {
    uint32_t source;
    uint64_t pid;
    ScoringParamKey() {
        memset(this, 0, sizeof(ScoringParamKey));
    }

    const char* to_string() {
        static char str[256];
        snprintf(str, 256, "source=%u, pid=%lu", source, pid);
        return str;
    }
};


enum VideoContexType {
    VIDEO_CONTEXT_TYPE_VID = 1,
    VIDEO_CONTEXT_TYPE_SHOW_ID,
    VIDEO_CONTEXT_TYPE_LIST_ID,
    VIDEO_CONTEXT_TYPE_FCHANNEL,
    VIDEO_CONTEXT_TYPE_SCHANNEL,
    VIDEO_CONTEXT_TYPE_OWNER_UID,
    VIDEO_CONTEXT_TYPE_KEYWORD,
    VIDEO_CONTEXT_TYPE_TITLE
};

struct VideoContextGradeKey
{
    uint8_t context_type;
    uint32_t fchannel;
    uint64_t context;

    VideoContextGradeKey()
    {
        memset(this, 0, sizeof(VideoContextGradeKey));
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "context_type=%u, fchannel=%u, context=%lu",
                context_type, fchannel, context);
        return str;
    }

};

struct VideoContextGradeValue
{
    float quality;

    VideoContextGradeValue()
    {
        quality = 0.0f;
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "quality=%f", quality);
        return str;
    }

};

struct SpotGradeKey
{
    uint32_t source;
    uint64_t pid;
    uint64_t app_id;

    SpotGradeKey()
    {
        memset(this, 0, sizeof(SpotGradeKey));
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "source=%u, pid=%lu, app_id=%lu",
                source, pid, app_id);
        return str;
    }

};

struct SpotGradeValue
{
    uint32_t grade;
    float quality;

    SpotGradeValue()
    {
        grade = 0;
        quality = 0.0f;
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "grade=%u, quality=%f", grade, quality);
        return str;
    }
};


#pragma pack(pop)

} // namespace model_updater
} // namespace poseidon

#endif // _MODEL_UPDATER_MODEL_API_STRUCTS_H_

