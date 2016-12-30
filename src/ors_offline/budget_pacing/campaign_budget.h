#ifndef _ORS_OFFLINE_BUDGET_PACING_H_
#define _ORS_OFFLINE_BUDGET_PACING_H_

#include <string>
#include <vector>
#include "json/json.h"
#include "types.h"

namespace poseidon {
    namespace ors_offline {
        enum ADS_TIME_TYPE {
            FULL_TYPE = 1,
            SELECT_HALF_HOURS = 2
        };

        enum ADS_BIDDING_MODE {
            ADS_NORMAL_BIDDING = 0,
            ADS_FIXED_BIDDING = 1
        };

        enum CAMPAIGN_TYPE {
            RTB = 1,
            PDB = 2
        };

        class CampaignBudget {
        private:
            UInt32 m_campaign_id;
            UInt64 m_budget;
            UInt32 m_ad_time_type;
            std::string m_post_hours;
            UInt32 m_target_adx;
        public:
            CampaignBudget() {}

            virtual ~CampaignBudget() {}

            // campaign_id
            void set_campaign_id(UInt32 v);

            UInt32 get_campaign_id();

            // budget
            void set_budget(UInt64 v);

            UInt64 get_budget();

            // ad_time_type
            void set_ad_time_type(UInt32 v);

            UInt32 get_ad_time_type();

            // post_hours
            void set_post_hours(const std::string &v, int week_day);

            std::string get_post_hours();

            // target_adx
            void set_target_adx(UInt32 v);

            UInt32 get_target_adx() const;

            // in mins
            int get_total_schedule_time();

            int get_total_posted_time(size_t mins_of_day);

            double get_schedule_budget(size_t mins_of_day);

            bool is_during_schedule_time(size_t mins_of_day);
        };

        //"pdb_data":
        // {"deal_id": "020160701",
        // "settle_price": 1600,
        // "total_exp": 100000,
        // "day_exp": 2040,
        // "campaign_quota": 1000,
        // "fill_rate": 85}
        class pdbCampaignBudget {
        private:
            UInt32 compaign_id;

            UInt32 target;
            UInt32 m_ad_time_type;
            std::string m_post_hours;

            // PDB specify data
            std::string deal_id;
            UInt32 exp;
            UInt32 quota;
            UInt32 fillrate;

        public:
            UInt32 getTarget() const;

            void setTarget(UInt32 target);

            UInt32 getM_ad_time_type() const;

            void setM_ad_time_type(UInt32 m_ad_time_type);

            const std::string &getM_post_hours() const;

            void setM_post_hours(const std::string &v, int week_day);

            UInt32 getCompaign_id() const;

            void setCompaign_id(UInt32 compaign_id);

            const std::string &getDeal_id() const;

            void setDeal_id(const std::string &deal_id);

            UInt32 getExp() const;

            void setExp(UInt32 exp);

            UInt32 getQuota() const;

            void setQuota(UInt32 quota);

            UInt32 getFillrate() const;

            void setFillrate(UInt32 fillrate);

            // in mins
            int get_total_schedule_time();

            int get_total_posted_time(size_t mins_of_day);

            double get_schedule_budget(size_t mins_of_day);

            bool is_during_schedule_time(size_t mins_of_day);


        };

    }// ors_offline
}// namespace poseidon

#endif // _ORS_OFFLINE_BUDGET_PACING_H_
