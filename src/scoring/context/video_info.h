
#ifndef SRC_SCORING_CONTEXT_VIDEO_INFO_H_
#define SRC_SCORING_CONTEXT_VIDEO_INFO_H_

#include "protocol/src/poseidon_proto.h"

namespace poseidon {

namespace scoring {

class VideoInfo {
public:
    int ResetInfo(uint32_t source, const rtb::Video& video);

    bool IsValid() {
        return _is_valid;
    }

    uint32_t FChannel() {
        return _fchannel;
    }

    bool HasFChannel() {
        return _fchannel != 0;
    }

    uint64_t VId() {
        return _vid;
    }

    bool HasVId() {
        return _vid != 0;
    }

    bool HasShowId() {
        return _show_id != 0;
    }

    uint32_t ShowId() {
        return _show_id;
    }

    const std::vector<uint64_t>& TitleSegments() {
        return _title_segments;
    }

    bool HasTitle() {
        return _title_segments.size() != 0;
    }

    bool hasSChannel() {
        return _schannels.size() != 0;
    }

    const std::vector<uint32_t>& SChannels() {
        return _schannels;
    }

    bool HasKeyWord() {
        return _keywords.size() != 0;
    }

    const std::vector<uint64_t>& Keywords() {
        return _keywords;
    }

    uint32_t VideoSiteId() {
        return _video_site_id;
    }


private:
    bool _is_valid;
    uint32_t _fchannel;
    uint64_t _vid;
    std::vector<uint32_t> _schannels;
    uint32_t _show_id;
    uint32_t _owner_id;
    std::vector<uint64_t> _title_segments;
    std::vector<uint64_t> _keywords;
    uint32_t _video_site_id;

    void reset();
};

}
}



#endif /* SRC_SCORING_CONTEXT_VIDEO_INFO_H_ */
