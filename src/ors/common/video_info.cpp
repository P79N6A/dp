/**
**/

//include STD C/C++ head files

//include third_party_lib head files
#include "src/ors/common/video_info.h"
#include "util/func.h"
#include "util/log.h"
#include "third_party/boost/include/boost/algorithm/string.hpp"
#include "src/ors/common/util.h"
#include "src/ors/common/common_def.h"

namespace poseidon
{
namespace ors
{

VideoInfo::VideoInfo()
{

}


VideoInfo::~VideoInfo()
{

}

bool VideoInfo::Bind(uint32_t adx_id, const rtb::Video& video)
{
    m_vid = 0;
    m_vid_str.clear();
    m_fchannel = 0;
    m_schannels.clear();
    m_schannels_str.clear();
    m_show_id = 0;
    m_showid_str.clear();
    m_owner_id = 0;
    m_ownerid_str.clear();
    m_title_segments.clear();
    m_keywords.clear();
    m_is_valid = false;
    m_fchannel_str.clear();
    m_keywords_str.clear();
    m_title_str.clear();
    m_video_site_id = 0;

    if (video.has_ext() && video.ext().has_content())
    {
        m_is_valid = true;
        if (video.ext().content().has_title())
        {
            const std::string& title = video.ext().content().title();
            if (!title.empty()) {
                m_title_str = title;
                std::string ftitle = filter_pun(title);
                std::vector<std::string> strs;
                boost::split(strs, ftitle, boost::is_any_of(" "));
                for (size_t i = 0; i < strs.size(); i++) {
                    boost::trim(strs[i]);
                    if (!strs[i].empty()) {
                        m_title_segments.push_back(util::Func::BytesHash64(strs[i].data(), strs[i].size()));
                        LOG_DEBUG("Video title segment %d = %s", i, strs[i].c_str());
                    }
                }
            }
            LOG_DEBUG("Video title = %s", title.c_str());
        }

        for (int i = 0; i < video.ext().content().keywords_size(); i++)
        {
            std::string keyword = video.ext().content().keywords(i);
            boost::trim(keyword);
            if (!keyword.empty())
            {
                m_keywords.push_back(util::Func::BytesHash64(keyword.data(), keyword.size()));
                m_keywords_str.push_back(keyword);
            }
            LOG_DEBUG("Video keyword %d= %s", i, keyword.c_str());
        }
    }

    if (video.has_ext() && video.ext().has_content() && video.ext().content().has_ext())
    {
        m_is_valid = true;
        for (int i = 0; i < video.ext().content().ext().direct_size(); i++)
        {
            const std::string& key = video.ext().content().ext().direct(i).key();
            const std::string& value = video.ext().content().ext().direct(i).value();
            if (key == "vid" && !value.empty())
            {
                m_vid_str = value;
                m_vid = util::Func::to_int(value);
                LOG_DEBUG("Video vid = %s", value.c_str());
                continue;
            }

            if (key == "s" && !value.empty())
            {
                m_showid_str = value;
                m_show_id = util::Func::to_int(value);
                LOG_DEBUG("Video show id = %s", value.c_str());
                continue;
            }

            if (key == "channel" && !value.empty())
            {
                m_fchannel_str = value;
                m_fchannel = util::Func::to_int(value);
                if (m_fchannel == 0)
                {
                    m_fchannel = util::Func::BytesHash32(value.data(), value.size());
                }

                if (adx_id == ADX_ID_IQIYI)
                {
                    m_video_site_id = VIDEO_SITE_IQIYI;
                }
                else if (adx_id == ADX_ID_YOUKU)
                {
                    if (m_fchannel_str.find_first_not_of("0123456789") == std::string::npos)
                    {
                        m_video_site_id = VIDEO_SITE_TUDOU;
                    }
                    else
                    {
                        m_video_site_id = VIDEO_SITE_YOUKU;
                    }
                }
                LOG_DEBUG("Video channel = %s, video_site_id = %d", value.c_str(), m_video_site_id);
                continue;
            }

            if(key == "usr" && !value.empty())
            {
                m_ownerid_str = value;
                m_owner_id = util::Func::to_int(value);
                LOG_DEBUG("Video usr = %s", value.c_str());
                continue;
            }

            if (key == "cs" && !value.empty())
            {
                boost::split(m_schannels_str, value, boost::is_any_of("|"));
                for (std::vector<std::string>::iterator iter = m_schannels_str.begin(); iter != m_schannels_str.end(); iter++)
                {
                    if (!(*iter).empty())
                    {
                        m_schannels.push_back(util::Func::to_int(*iter));
                    }
                }
                LOG_DEBUG("Video cs = %s", value.c_str());
            }
        }
    }

    LOG_DEBUG("VideoInfo Bind OK!");
    return true;
}

} // namespace ors
} // namespace poseidon

