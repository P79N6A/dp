#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include <string>
#include <vector>
#include <tr1/unordered_map>

#include <muduo/base/Logging.h>

template <typename DataType> class DataItem
{
    public:
        DataItem()
        {
            timestamp = 0;
        }
        std::vector<DataType> vec;
        // 数据在此时间戳内是有效的, 否则视为失效数据
        time_t timestamp;
};

template <typename Key, typename Value> class CacheMap
{
    public:
        CacheMap(){}
        ~CacheMap(){}
        int getSize()
        {
            return m_map.size();
        }
        bool find(const Key& k, std::vector<Value>& output)
        {
            time_t now = time(NULL);
            typename HashMap::const_iterator it = m_map.find(k);
            if (it != m_map.end() && now < it->second.timestamp)
            {
                output = it->second.vec;
                LOG_DEBUG << "Find Cache Value vector size: " << output.size();
                return true;
            }
            else
            {
                return false;
            }
        }
        // valid time 5 min
        bool store(const Key& k, std::vector<Value>& input, int validTimeSlot = 300)
        {
            time_t timestamp = time(NULL) + validTimeSlot;
            m_map[k].timestamp = timestamp;
            // must be cleared first
            m_map[k].vec.clear();
            for (typename std::vector<Value>::iterator it = input.begin();
                it != input.end(); ++it)
            {
                m_map[k].vec.push_back(*it);
            }
            LOG_DEBUG << "Store to Cache Value vector size: " << m_map[k].vec.size();
            return true;
        }
    private:
        typedef std::tr1::unordered_map< Key, DataItem<Value> > HashMap;
        HashMap m_map;
};
