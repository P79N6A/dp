/**
**/

#ifndef _ORS_SHM_STAT_H_
#define _ORS_SHM_STAT_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "util/hash_shm.h"
#include "util/func.h"
#include "util/shm.h"
#include "util/log.h"
#include "util/date_time.h"
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace ors
{

#pragma pack(push, 1)

struct TPropertyKey
{
    int property_type;
    uint64_t property_id;
    TPropertyKey()
    {
        property_type = 0;
        property_id = 0;
    }

    bool operator==(const TPropertyKey& other)
    {
        return memcmp(this, &other, sizeof(TPropertyKey)) == 0;
    }

    TPropertyKey& operator=(const TPropertyKey& other)
    {
        this->property_type = other.property_type;
        this->property_id = other.property_id;

        return *this;
    }

    const char* to_string()
    {
        static char str[256];
        snprintf(str, 256, "property_type=%d, property_id=%lu", property_type, property_id);
        return str;
    }
};

struct TPropertyValue
{
    int second_stat[2];
    int minute_stat[2];
    int hour_stat[2];
    int day_stat[2];
    int sec_idx;
    int min_idx;
    int hour_idx;
    int day_idx;
    uint32_t second;
    const char* to_string()
    { 
        static char str[256];
        snprintf(str, 256, "second=%u, second_stat=[%u,%u], minute_stat=[%u,%u], hour_stat=[%u,%u], day_stat=[%u,%u]", 
                second,
                second_stat[0],second_stat[1],
                minute_stat[0], minute_stat[1],
                hour_stat[0], hour_stat[1],
                day_stat[0], day_stat[1]);
        return str;
    }
};
#pragma pack(pop)
 
class ShmStat : public util::HashShm<TPropertyKey, TPropertyValue>
{

public:
    ShmStat()
    {

    }
    virtual ~ShmStat()
    {

    }

    virtual bool InitShm()
    {
        void* buffer = util::shm::ShmCreate(20160620, 64*1024*1024, 0666);
        if (!buffer)
        {
           LOG_ERROR("ShmAttach Failed!");
           return false;
        }

        this->Init(buffer, 64*1024*1024);
        LOG_INFO("InitShm OK");
        return true;
    }

    void Add(int property_type, uint64_t property_id, int value)
    {
        static TPropertyKey key;
        int sec_idx = util::DateTime::get_mutable_instance().GetUnixTime() % 2;
        int min_idx = util::DateTime::get_mutable_instance().GetTimeMinute() % 2;
        int hour_idx = util::DateTime::get_mutable_instance().GetTimeHour() % 2;
        int day_idx = util::DateTime::get_mutable_instance().GetWeekDay() % 2;

        key.property_type = property_type;
        key.property_id = property_id;

        uint32_t hash = util::Func::BytesHash32((char*)&key, sizeof(TPropertyKey));
        int index = hash % m_hash_head->capacity;
        int offset = m_hash_index[index];

        bool is_conflict = false;
        while (offset != 0)
        {
            HashNode* conflict_node =  &m_hash_node[offset];
            if (conflict_node->key == key)
            {
                conflict_node->value.second = util::DateTime::get_mutable_instance().GetUnixTime();
                if (conflict_node->value.sec_idx == sec_idx) 
                {
                    conflict_node->value.second_stat[sec_idx] += value;
                }
                else
                {
                    conflict_node->value.sec_idx = sec_idx;
                    conflict_node->value.second_stat[sec_idx] = value;
                }

                if (conflict_node->value.min_idx == min_idx)
                {
                    conflict_node->value.minute_stat[min_idx] += value;
                }
                else
                {
                    conflict_node->value.min_idx = min_idx;
                    conflict_node->value.minute_stat[min_idx] = value;
                }

                if (conflict_node->value.hour_idx == hour_idx)
                {
                    conflict_node->value.hour_stat[hour_idx] += value;
                }
                else
                {   
                    conflict_node->value.hour_idx = hour_idx;
                    conflict_node->value.hour_stat[hour_idx] = value;
                }

                if (conflict_node->value.day_idx == day_idx)
                {
                    conflict_node->value.day_stat[day_idx] += value;
                }
                else
                {
                    conflict_node->value.day_idx = day_idx;
                    conflict_node->value.day_stat[day_idx] = value;
                }
                return;
            }
            
            offset = conflict_node->link;
            is_conflict = true;
        }

        if (is_conflict)
        {
            m_hash_head->conflict_count++;
        }

        HashNode *new_node = &m_hash_node[m_hash_head->empty_offset];
        new_node->hash = hash;
        new_node->key = key;

        new_node->value.second = util::DateTime::get_mutable_instance().GetUnixTime();

        new_node->value.sec_idx = sec_idx;
        new_node->value.min_idx = min_idx;
        new_node->value.hour_idx = hour_idx;
        new_node->value.day_idx = day_idx;

        new_node->value.second_stat[sec_idx] = value;
        new_node->value.minute_stat[min_idx] = value;
        new_node->value.hour_stat[hour_idx]= value;
        new_node->value.day_stat[day_idx] = value;
        new_node->link = m_hash_index[index];
        new_node->valid = true;
        m_hash_index[index] = m_hash_head->empty_offset;

        m_hash_head->empty_offset += 1;
        m_hash_head->current_size += 1;

    }


protected:

};
} // namespace ors
} // namespace poseidon

#endif // _ORS_SHM_STAT_H_

