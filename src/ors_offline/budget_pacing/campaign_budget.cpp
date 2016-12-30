#include "campaign_budget.h"

using namespace std;

namespace poseidon
{
	namespace ors_offline
	{
		void CampaignBudget::set_campaign_id(UInt32 v)
		{
			m_campaign_id = v;
		}

		UInt32 CampaignBudget::get_campaign_id()
		{
			return m_campaign_id;
		}

		void CampaignBudget::set_budget(UInt64 v)
		{
			m_budget = v;
		}

		UInt64 CampaignBudget::get_budget()
		{
			return m_budget;
		}

		void CampaignBudget::set_ad_time_type(UInt32 v)
		{
			m_ad_time_type = v;
		}
		UInt32 CampaignBudget::get_ad_time_type()
		{
			return m_ad_time_type;
		}
		void CampaignBudget::set_post_hours(const string& v, int week_day)
		{
			int day_slots = 24 * 2;
			m_post_hours = v.substr(week_day * day_slots, day_slots);
		}

		string CampaignBudget::get_post_hours()
		{
			return m_post_hours;
		}

		void CampaignBudget::set_target_adx(UInt32 v)
		{
			m_target_adx = v;
		}

		UInt32 CampaignBudget::get_target_adx() const
		{
			return m_target_adx;
		}

		int CampaignBudget::get_total_schedule_time()
		{
			if (m_ad_time_type == FULL_TYPE)
			{
				return 24 * 60;
			}
			int total_schedule_time = 0;
			for (size_t i = 0; i < m_post_hours.size(); i++)
			{
				if (m_post_hours[i] == '1')
				{
					total_schedule_time += 30;
				}
			}
			return total_schedule_time;
		}

		int CampaignBudget::get_total_posted_time(size_t mins_of_day)
		{
			if (m_ad_time_type == FULL_TYPE)
			{
				return mins_of_day;
			}
			
			int total_posted_time = 0;
			for (size_t i = 0; i < m_post_hours.size(); i++)
			{
				if (m_post_hours[i] == '1')
				{
					if ((i + 1) * 30 < mins_of_day)
					{
						total_posted_time += 30;
					}
					else if (i * 30 < mins_of_day)
					{
						total_posted_time += mins_of_day - i * 30;
						break;
					}
				}
			}
			return total_posted_time;
		}

		double CampaignBudget::get_schedule_budget(size_t mins_of_day)
		{
			int total_schedule_time = get_total_schedule_time();
			int total_posted_time = get_total_posted_time(mins_of_day);
			if (total_schedule_time == 0)
			{
				return 0;
			}

			return (double)total_posted_time / (double)total_schedule_time * m_budget;
		}

		bool CampaignBudget::is_during_schedule_time(size_t mins_of_day)
		{
			if (m_ad_time_type == FULL_TYPE)
			{
				return true;
			}
			size_t slot = (mins_of_day - 1) / 30;
			if (slot < m_post_hours.size() && m_post_hours[slot] == '1')
			{
				return true;
			}
			return false;
		}


		UInt32 pdbCampaignBudget::getCompaign_id() const {
			return compaign_id;
		}

		void pdbCampaignBudget::setCompaign_id(UInt32 compaign_id) {
			pdbCampaignBudget::compaign_id = compaign_id;
		}

		const string &pdbCampaignBudget::getDeal_id() const {
			return deal_id;
		}

		void pdbCampaignBudget::setDeal_id(const string &deal_id) {
			pdbCampaignBudget::deal_id = deal_id;
		}

		UInt32 pdbCampaignBudget::getExp() const {
			return exp;
		}

		void pdbCampaignBudget::setExp(UInt32 exp) {
			pdbCampaignBudget::exp = exp;
		}

		UInt32 pdbCampaignBudget::getQuota() const {
			return quota;
		}

		void pdbCampaignBudget::setQuota(UInt32 quota) {
			pdbCampaignBudget::quota = quota;
		}

		UInt32 pdbCampaignBudget::getFillrate() const {
			return fillrate;
		}

		void pdbCampaignBudget::setFillrate(UInt32 fillrate) {
			pdbCampaignBudget::fillrate = fillrate;
		}

		UInt32 pdbCampaignBudget::getTarget() const {
			return target;
		}

		void pdbCampaignBudget::setTarget(UInt32 target) {
			pdbCampaignBudget::target = target;
		}

		UInt32 pdbCampaignBudget::getM_ad_time_type() const {
			return m_ad_time_type;
		}

		void pdbCampaignBudget::setM_ad_time_type(UInt32 m_ad_time_type) {
			pdbCampaignBudget::m_ad_time_type = m_ad_time_type;
		}

		const string &pdbCampaignBudget::getM_post_hours() const {
			return m_post_hours;
		}


        void pdbCampaignBudget::setM_post_hours(const string& v, int week_day)
        {
            int day_slots = 24 * 2;
            m_post_hours = v.substr(week_day * day_slots, day_slots);
        }


		int pdbCampaignBudget::get_total_schedule_time()
		{
			if (m_ad_time_type == FULL_TYPE)
			{
				return 24 * 60;
			}
			int total_schedule_time = 0;
			for (size_t i = 0; i < m_post_hours.size(); i++)
			{
				if (m_post_hours[i] == '1')
				{
					total_schedule_time += 30;
				}
			}
			return total_schedule_time;
		}

		int pdbCampaignBudget::get_total_posted_time(size_t mins_of_day)
		{
			if (m_ad_time_type == FULL_TYPE)
			{
				return mins_of_day;
			}

			int total_posted_time = 0;
			for (size_t i = 0; i < m_post_hours.size(); i++)
			{
				if (m_post_hours[i] == '1')
				{
					if ((i + 1) * 30 < mins_of_day)
					{
						total_posted_time += 30;
					}
					else if (i * 30 < mins_of_day)
					{
						total_posted_time += mins_of_day - i * 30;
						break;
					}
				}
			}
			return total_posted_time;
		}

		double pdbCampaignBudget::get_schedule_budget(size_t mins_of_day)
		{
			int total_schedule_time = get_total_schedule_time();
			int total_posted_time = get_total_posted_time(mins_of_day);
			if (total_schedule_time == 0)
			{
				return 0;
			}

			return (double)total_posted_time / (double)total_schedule_time * exp;
		}

		bool pdbCampaignBudget::is_during_schedule_time(size_t mins_of_day)
		{
			if (m_ad_time_type == FULL_TYPE)
			{
				return true;
			}
			size_t slot = (mins_of_day - 1) / 30;
			if (slot < m_post_hours.size() && m_post_hours[slot] == '1')
			{
				return true;
			}
			return false;
		}

	}// ors_offline
}// namespace poseidon
