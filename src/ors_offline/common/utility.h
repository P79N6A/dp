#ifndef POSEIDON_ORS_OFFLINE_COMMON_H
#define POSEIDON_ORS_OFFLINE_COMMON_H

#include <time.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <dirent.h>


inline int get_week_day(time_t t)
{
    char week_day[10];
    strftime(week_day, sizeof(week_day), "%w", localtime(&t));

    return atoi(week_day);
}

inline size_t get_mins_of_day(time_t t)
{
    char hours[10];
    strftime(hours, sizeof(hours), "%H", localtime(&t));
    char mins[10];
    strftime(mins, sizeof(mins), "%M", localtime(&t));

    return atoi(hours) * 60 + atoi(mins) + 1;
}

inline std::string getYmd(time_t t)
{
    char day[9];
    strftime(day, 9, "%Y%m%d", localtime(&t));

    return std::string(day);
}

inline std::string getY_m_d(time_t t)
{
    char day[11];
    strftime(day, 11, "%Y-%m-%d", localtime(&t));

    return std::string(day);
}


inline std::string getYmdH(time_t t)
{
    char ymdh[11];
    strftime(ymdh, 11, "%Y%m%d%H", localtime(&t));

    return std::string(ymdh);
}

inline std::string getYmdHMS(time_t t)
{
    char full_t[15];
    strftime(full_t, 15, "%Y%m%d%H%M%S", localtime(&t));

    return std::string(full_t);
}

inline std::string getH(time_t t)
{
    char hour[3];
    strftime(hour, 3, "%H", localtime(&t));

    return std::string(hour);
}

inline std::string getM(time_t t)
{
    char mins[3];
    strftime(mins, 3, "%M", localtime(&t));

    return std::string(mins);
}

inline std::vector<std::string> split(const std::string& src, const std::string& delimiter)
{
    std::vector<std::string> strs;
    int delimiter_len = delimiter.size();

    int index = -1, last_pos = 0;
    while (-1 != (index = src.find(delimiter, last_pos)))
    {
        strs.push_back(src.substr(last_pos, index - last_pos));
        last_pos = index + delimiter_len;
    }
    std::string last_str = src.substr(last_pos);
    if (!last_str.empty())
    {
        strs.push_back(last_str);
    }

    return strs;
}

inline int cntCharInStr(const std::string& str, char ch)
{
    int cnt = 0;
    for (size_t i = 0; i < str.size(); i++)
    {
        if (str[i] == ch)
        {
            cnt++;
        }
    }
    return cnt;
}

inline bool dir_exists(const char* path)
{
    if (NULL == path)
        return false;

    DIR* p_dir = opendir(path);
    if (NULL != p_dir)
    {
        closedir(p_dir);
        return true;
    }
    return false;
}

#endif //POSEIDON_ORS_OFFLINE_COMMON_H

