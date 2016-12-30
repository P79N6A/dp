/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "src/model_updater/model/video_context_grade_handler.h"
#include "src/model_updater/api/structs.h"
#include "third_party/cityhash/include/city.h"
#include "util/log.h"
#include "util/proto_helper.h"
#include "util/func.h"


namespace poseidon
{
namespace model_updater
{

VideoContextGradeHandler::VideoContextGradeHandler()
{

}

VideoContextGradeHandler::~VideoContextGradeHandler()
{

}

bool VideoContextGradeHandler::Update()
{  
    ors::AlgoModelData algo_model_data;
    if(!util::ParseProtoFromBinaryFormatFile(m_watch_file.c_str(), algo_model_data.mutable_video_context_grade_model_data()))
    {
        LOG_ERROR("ParseProtoFromTextFormatFile %s Failed!", m_watch_file.c_str());
        return false;
    }

    VideoContextGradeKey key;
    VideoContextGradeValue value;
    const ors::VideoContextGradeModel& video_context_grade_model = algo_model_data.video_context_grade_model_data();
    for (int i = 0; i < video_context_grade_model.items_size(); i++)
    {
        const ors::VideoContextGradeItem& item = video_context_grade_model.items(i);
        key.source = 2;
        if (item.has_source())
        {
            key.source = item.source();
        }

        key.fchannel = 0;
        if (item.has_fchannel())
        {
            key.fchannel = util::Func::to_int(item.fchannel().c_str());
            if(key.fchannel == 0) {
                key.fchannel = util::Func::BytesHash32(item.fchannel().data(), item.fchannel().size());
            }
        }
        
        value.quality = item.quality();
        if (item.has_video_id())
        {
            key.context = item.video_id();
            key.context_type = VIDEO_CONTEXT_TYPE_VID;
            this->SetMemkv((const char*)&key, sizeof(VideoContextGradeKey), (const char*)&value, sizeof(VideoContextGradeValue));
            continue;
        }

        if (item.has_show_id())
        {
            key.context = item.show_id();
            key.context_type = VIDEO_CONTEXT_TYPE_SHOW_ID;
            this->SetMemkv((const char*)&key, sizeof(VideoContextGradeKey), (const char*)&value, sizeof(VideoContextGradeValue));
            continue;
        }

        if (item.has_list_id())
        {
            key.context = item.list_id();
            key.context_type = VIDEO_CONTEXT_TYPE_LIST_ID;
            this->SetMemkv((const char*)&key, sizeof(VideoContextGradeKey), (const char*)&value, sizeof(VideoContextGradeValue));
            continue;
        }

        if (item.has_owner_uid())
        {
            key.context = item.owner_uid();
            key.context_type = VIDEO_CONTEXT_TYPE_OWNER_UID;
            this->SetMemkv((const char*)&key, sizeof(VideoContextGradeKey), (const char*)&value, sizeof(VideoContextGradeValue));
            continue;
        }

        if (item.has_schannel())
        {
            key.context_type = VIDEO_CONTEXT_TYPE_SCHANNEL;
            key.context = item.schannel();
            this->SetMemkv((const char*)&key, sizeof(VideoContextGradeKey), (const char*)&value, sizeof(VideoContextGradeValue));
            continue;
        }

        if (item.has_keyword())
        {
            key.context_type = VIDEO_CONTEXT_TYPE_KEYWORD;
            key.context = util::Func::BytesHash64(item.keyword().data(), item.keyword().size());
            this->SetMemkv((const char*)&key, sizeof(VideoContextGradeKey), (const char*)&value, sizeof(VideoContextGradeValue));
            continue;
        }

        if (item.has_title_segment())
        {
            key.context_type = VIDEO_CONTEXT_TYPE_TITLE;
            key.context = util::Func::BytesHash64(item.title_segment().data(), item.title_segment().size());
            this->SetMemkv((const char*)&key, sizeof(VideoContextGradeKey), (const char*)&value, sizeof(VideoContextGradeValue));
            continue;
        }

        if (item.has_fchannel())
        {
            key.context_type = VIDEO_CONTEXT_TYPE_FCHANNEL;
            key.context = key.fchannel;
            this->SetMemkv((const char*)&key, sizeof(VideoContextGradeKey), (const char*)&value, sizeof(VideoContextGradeValue));
        }
    }

    LOG_INFO("Update OK!file items=%d", video_context_grade_model.items_size());
    return true;
}


} // namespace ors
} // namespace poseidon

