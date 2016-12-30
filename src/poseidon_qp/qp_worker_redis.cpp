#include "qp_worker.h"
#include "attr.h"
#include "qp_toolkits.h"

namespace poseidon {
namespace qp {

void QpWorker::get_user_id(boost::shared_ptr<QpContext> context) {
    int bucket_key = util::Func::crc32(context->imei_md5.c_str(),
            context->imei_md5.length()) / 128;
    string bucket_key_str = util::Func::to_str(bucket_key);
    redisAsyncContext * redis_context = _redis_client.get_redis_context(
            bucket_key_str);
    if (redis_context == NULL) {
        LOG_ERROR("%s get redis context error", context->id.c_str());
        context->got_redis_tags = true;
        done(context);
        return;
    }
    RedisOnReply *on_reply = RedisOnReply::new_instance(
            boost::bind(&QpWorker::on_got_user_id, this, context->id, _1));
    if (redisAsyncCommand(redis_context, redis_command_cb, (void *) on_reply,
            "hget %d %s", bucket_key, context->imei_md5.c_str()) != 0) {
        LOG_ERROR("%s redis command error", context->id.c_str());
        MON_ADD(ATTR_SEND_CMD_ERROR, 1);
        context->got_redis_tags = true;
        done(context);
        return;
    } else {
        MON_ADD(ATTR_REDIS_SND_CMD, 1);
    }
}

void QpWorker::on_got_user_id(const string &context_id, redisReply *reply) {
    ContextMapIter iter = _context_map.find(context_id);
    if (iter == _context_map.end())
        return;
    bool snd_tag_cmd = false;
    boost::shared_ptr<QpContext> context = iter->second;
    if (reply == NULL) {

    } else if (reply->type == REDIS_REPLY_STRING) {
        context->imei = reply->str;
        LOG_DEBUG("get user imei is : %s", context->imei.c_str());
        snd_tag_cmd = true;
    } else if (reply->type == REDIS_REPLY_NIL) {

    } else {
        LOG_ERROR("%s redis reply error,%d", context->id.c_str(), reply->type);
        MON_ADD(ATTR_REDIS_ON_ERROR, 1);
    }

    context->interval("user_id_time");
    if (snd_tag_cmd) {
        MON_ADD(ATTR_USER_ID_HAVE_DATA, 1);
        get_user_tag(context);
    } else {
        context->got_redis_tags = true;
        done(context);
    }
}

void QpWorker::get_user_tag(boost::shared_ptr<QpContext> context) {
    string bucket_key = QpToolkits::get_bucket_key(context->imei);
    string bucket_index = QpToolkits::get_bucket_index(context->imei);
    redisAsyncContext * redis_context = _redis_client.get_redis_context(
            bucket_key);
    if (redis_context == NULL) {
        LOG_ERROR("%s get redis context error", context->id.c_str());
        context->got_redis_tags = true;
        done(context);
        return;
    }
    RedisOnReply *on_reply = RedisOnReply::new_instance(
            boost::bind(&QpWorker::on_got_user_tag, this, context->id, _1));
    if (redisAsyncCommand(redis_context, redis_command_cb, (void *) on_reply,
            "hget %s %s", bucket_key.c_str(), bucket_index.c_str()) != 0) {
        LOG_ERROR("%s redis command error", context->id.c_str());
        MON_ADD(ATTR_SEND_CMD_ERROR, 1);
        context->got_redis_tags = true;
        done(context);
        return;
    } else {
        MON_ADD(ATTR_REDIS_SND_CMD, 1);
    }
}

void QpWorker::on_got_user_tag(const string &context_id, redisReply *reply) {
    ContextMapIter iter = _context_map.find(context_id);
    if (iter == _context_map.end())
        return;
    boost::shared_ptr<QpContext> context = iter->second;
    if (reply == NULL) {

    } else if (reply->type == REDIS_REPLY_STRING) {
        dmp::DmpUserData dmp_user_data;
        if (dmp_user_data.ParseFromArray(reply->str, reply->len)) {
            MON_ADD(ATTR_USER_TAG_HAVE_DATA, 1);
            for (int i = 0; i < dmp_user_data.tag_datas_size(); i++) {
                for (int j = 0; j < dmp_user_data.tag_datas(i).values_size();
                        j++) {
                    context->add_tag_value(dmp_user_data.tag_datas(i).tag_no(),
                            dmp_user_data.tag_datas(i).values(j));
                }
            }
        } else {
            LOG_ERROR("%s redis data parse error", context->id.c_str());
            MON_ADD(ATTR_REDIS_ON_ERROR, 1);
        }
    } else if (reply->type == REDIS_REPLY_NIL) {

    } else {
        LOG_ERROR("%s redis reply error,%d", context->id.c_str(), reply->type);
        MON_ADD(ATTR_REDIS_ON_ERROR, 1);

    }
    context->got_redis_tags = true;
    context->interval("user_tag_time");
    done(context);
    return;
}

}
}
