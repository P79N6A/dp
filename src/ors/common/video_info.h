/**
**/

#ifndef _ORS_VIDEO_INFO_H_
#define _ORS_VIDEO_INFO_H_
//include STD C/C++ head files
#include <stdint.h>
#include <string>
#include <vector>

//include third_party_lib head files
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace ors
{
class VideoInfo
{

public:
    VideoInfo();
    virtual ~VideoInfo();
    virtual bool Bind(uint32_t adx_id, const rtb::Video& video);

    bool IsValid()
    {
        return m_is_valid;
    }

    uint32_t GetVId()
    {
        return m_vid;
    }

    bool HasVId()
    {
        return m_vid != 0;
    }

    uint32_t GetFChannel()
    {
        return m_fchannel;
    }

    bool HasFChannel()
    {
        return m_fchannel != 0;
    }

    bool HasShowId()
    {
        return m_show_id != 0;
    }

    uint32_t GetShowId()
    {
        return m_show_id;
    }

    uint32_t GetOwnerId()
    {
        return m_owner_id;
    }

    const std::vector<uint64_t>& GetTitleSegments()
    {
        return m_title_segments;
    }

    bool HasTitle()
    {
        return m_title_segments.size() != 0;
    }

    bool hasSChannel()
    {
        return m_schannels.size() != 0;
    }

    const std::vector<uint32_t>& GetSChannels()
    {
        return m_schannels;
    }

    bool HasKeyWord()
    {
        return m_keywords.size() != 0;
    }

    const std::vector<uint64_t>& GetKeywords()
    {
        return m_keywords;
    }

    const std::string& GetFChannelStr()
    {
        return m_fchannel_str;
    }

    const std::vector<std::string>& GetSChannelsStr()
    {
        return m_schannels_str;
    }

    const std::vector<std::string>& GetKeywordsStr()
    {
        return m_keywords_str;
    }

    const std::string& GetTitleStr()
    {
        return m_title_str;
    }

    const std::string& GetVIdStr()
    {
        return m_vid_str;
    }

    const std::string& GetShowIdStr()
    {
        return m_showid_str;
    }

    const std::string& GetOwnerIdStr()
    {
        return m_ownerid_str;
    }

    uint32_t GetVideoSiteId()
    {
        return m_video_site_id;
    }

protected:


protected:
    uint32_t m_vid;
    uint32_t m_fchannel;
    std::vector<uint32_t> m_schannels;
    uint32_t m_show_id;
    uint32_t m_owner_id;
    std::vector<uint64_t> m_title_segments;
    std::vector<uint64_t> m_keywords;

    bool m_is_valid;

    std::string m_title_str;
    std::vector<std::string> m_keywords_str;
    std::string m_fchannel_str; 
    std::vector<std::string> m_schannels_str;
    std::string m_vid_str;
    std::string m_showid_str;
    std::string m_ownerid_str;

    uint32_t m_video_site_id;

};


} // namespace ors
} // namespace poseidon

#endif // _ORS_VIDEO_INFO_H_

