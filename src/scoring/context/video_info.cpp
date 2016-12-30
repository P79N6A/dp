#include "video_info.h"
#include <boost/algorithm/string.hpp>
#include "src/scoring/common/common.h"

namespace poseidon {

namespace scoring {

int VideoInfo::ResetInfo(uint32_t source, const rtb::Video& video) {
    reset();

    using namespace std;

    vector<string> keywords;
    if (video.has_ext() && video.ext().has_content()) {
        _is_valid = true;
        if (video.ext().content().has_title()) {
            const string& title = video.ext().content().title();
            if (!title.empty()) {
                string ftitle = filter_pun(title);
                vector<string> strs = split(ftitle, " ");
                for (size_t i = 0; i < strs.size(); i++) {
                    boost::trim(strs[i]);
                    if (!strs[i].empty()) {
                        _title_segments.push_back(
                                util::Func::BytesHash64(strs[i].data(),
                                        strs[i].size()));
                        LOG_DEBUG("Video title segment %d = %s", i,
                                strs[i].c_str());
                    }
                }
            }
            LOG_DEBUG("Video title = %s", title.c_str());
        }

        for (int i = 0; i < video.ext().content().keywords_size(); i++) {
            const string& keyword = video.ext().content().keywords(i);
            if (!keyword.empty()) {
                if (ADX_ID_IQIYI == source) {
                    keywords.push_back(keyword);
                }
                _keywords.push_back(
                        util::Func::BytesHash64(keyword.data(),
                                keyword.size()));
            }
            LOG_DEBUG("Video keyword %d= %s", i, keyword.c_str());
        }
    }

    if (video.has_ext() && video.ext().has_content()
            && video.ext().content().has_ext()) {
        _is_valid = true;

        string showid = "", cnl = "";

        for (int i = 0; i < video.ext().content().ext().direct_size(); i++) {
            const string& key = video.ext().content().ext().direct(i).key();
            const string& value = video.ext().content().ext().direct(i).value();
            if (key == "vid" && !value.empty()) {
                _vid = util::Func::to_int(value);
                LOG_DEBUG("Video vid = %s", value.c_str());
                continue;
            }

            if (key == "s" && !value.empty()) {
                if (source == ADX_ID_IQIYI) {
                    showid = value;
                }
                _show_id = util::Func::to_int(value);
                LOG_DEBUG("Video show id = %s", value.c_str());
                continue;
            }

            if (key == "channel" && !value.empty()) {
                if (source == ADX_ID_IQIYI) {
                    cnl = value;
                }
                _fchannel = util::Func::to_int(value);
                if (_fchannel == 0) {
                    _fchannel = util::Func::BytesHash32(value.data(),
                            value.size());
                }

                if (source == ADX_ID_IQIYI) {
                    _video_site_id = VIDEO_SITE_IQIYI;
                } else if (source == ADX_ID_YOUKU) {
                    if (value.find_first_not_of("0123456789") == string::npos) {
                        _video_site_id = VIDEO_SITE_TUDOU;
                    } else {
                        _video_site_id = VIDEO_SITE_YOUKU;
                    }
                }
                LOG_DEBUG("Video channel = %s, video_site_id = %d",
                        value.c_str(), _video_site_id);
                continue;
            }

            if (key == "cs" && !value.empty()) {
                vector<string> strs = split(value, "|");
                for (vector<string>::iterator iter = strs.begin();
                        iter != strs.end(); iter++) {
                    if (!(*iter).empty()) {
                        _schannels.push_back(util::Func::to_int(*iter));
                    }
                }
                LOG_DEBUG("Video cs = %s", value.c_str());
            }
        }

        //iqiyi组装vid
        if (ADX_ID_IQIYI == source) {
            stringstream ss;
            ss << "cnl_" << cnl << "&keywords_";
            for (int i=0; i < keywords.size(); i++) {
                ss << keywords[i] << "|";
            }
            ss << "&show_id_" << showid;
            string iqiyi_vid_str = ss.str();
            _vid = util::Func::BytesHash64(iqiyi_vid_str.data(),
                    iqiyi_vid_str.size());
            LOG_DEBUG("Scoring iqiyi_vid_str: %s, vid: %lu", iqiyi_vid_str.c_str(), _vid);
        }
    }

    return 0;
}

void VideoInfo::reset() {
    _is_valid = false;
    _fchannel = 0;
    _vid = 0;
    _schannels.clear();
    _show_id = 0;
    _title_segments.clear();
    _keywords.clear();
    _video_site_id = 0;
}

}
}

