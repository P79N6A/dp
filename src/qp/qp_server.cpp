/**
 **/

#include "qp_server.h"
#include <unistd.h>
#include <map>
#include "protocol/src/poseidon_proto.h"
#include "util/log.h"
#include "redis_access.h"
#include "hiredis.h"
#include <boost/algorithm/string.hpp>
#include "util/util_str.h"
#include "util/func.h"
#include "dmp_util.h"
#include "monitor_api.h"
#include "attr.h"
#include "config.h"

namespace poseidon
{
namespace qp
{

/**
 * @brief               process req package
 **/
int QpServer::handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
{
    MON_ADD(ATTR_QP_REQ, 1);
    int rt=0;
    SessData * sess=NULL;
    do{
        sess = alloc_sess();
        if(sess == NULL )
        {
            LOG_ERROR("new SessData return NULL");
            rt=-1;
            break;
        }
        memcpy(&(sess->addr), &client_addr, sizeof(struct sockaddr_in) );
        QPRequest & qpreq=sess->qpreq;
        QPResponse & qprsp=sess->qprsp;
        if(!qpreq.ParseFromArray(buf, len))
        {
            MON_ADD(ATTR_QP_PARSE_ERR, 1);
            LOG_ERROR("new SessData return NULL");
            rt=-1;
            break;
        }

        LOG_DEBUG("pid[%d]qpreq[%s]", getpid(), qpreq.DebugString().c_str());

        if(!qpreq.has_device() || !qpreq.device().has_id())
        {
            MON_ADD(ATTR_QP_NO_DEV_ID, 1);
            qprsp.set_error_code(common::ERROR_NO_RESULT);
            reply_client(sess);
            rt=-1;
            break;
        }
        sess->imei=qpreq.device().id();

        rt=send_get_user_tag(sess);
        if(rt != 0)
        {
            LOG_ERROR("send_get_user_tag error[%d]", rt);
            qprsp.set_error_code(common::ERROR_NO_RESULT);
            reply_client(sess);
            rt=-1;
            break;
        }
        rt=send_get_user_game_tag(sess);
        if(rt != 0)
        {
            LOG_ERROR("send_get_user_game_tag error[%d]", rt);
            qprsp.set_error_code(common::ERROR_NO_RESULT);
            reply_client(sess);
            rt=-1;
            break;
        }

        if(qpreq.device().has_brand() && qpreq.device().has_model() )
        {
            rt=send_get_device_price(sess);
            if(rt != 0)
            {
                LOG_ERROR("send_get_device_price error[%d]", rt);
                qprsp.set_error_code(common::ERROR_NO_RESULT);
                reply_client(sess);
                rt=-1;
                break;
            }
        }
        //
        //

    }while(0);
    if(rt != 0)
    {
        if(sess != NULL)
        {
            free_sess(sess);
        }
    }
    return rt;
}


int QpServer::reply_client(SessData * sess)
{
    int rt=0;
    do{
        QPResponse & qprsp=sess->qprsp;
        QPRequest & qpreq=sess->qpreq;
        if(qpreq.has_session_id())
        {
            qprsp.set_session_id(qpreq.session_id());
        }

        if(qpreq.has_trace_id())
        {
            qprsp.set_trace_id(qpreq.trace_id());
        }

        char hostname[256];
        memset(hostname, 0x00, 256);
        if( gethostname(hostname, 256) == 0 )
        {
            qprsp.set_hostname(hostname);
        }

        std::string sendbuf;
        if(!qprsp.SerializeToString(&sendbuf))
        {
            MON_ADD(ATTR_PACK_ERROR, 1);
            LOG_ERROR("SerializeToString error");
            rt=-1;
            break;
        }
        LOG_DEBUG("qprsp[%s]\n", qprsp.DebugString().c_str());

        if(!Config::get_mutable_instance().is_dumb())
        {
            send_pkg(sendbuf.c_str(), sendbuf.length(), sess->addr);
        }
        MON_ADD(ATTR_QP_RSP, 1);
    }while(0);
    free_sess(sess);
    return rt;
}


int QpServer::send_get_user_tag(SessData * sess)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(RA_USER);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        util::UtilStr ustr;
        ustr.format("hget user_perspective %s", sess->imei.c_str());
        LOG_DEBUG("cmd[%s]", ustr.str().c_str());
        std::string trace_id;
        if(sess->qpreq.has_trace_id())
          trace_id=sess->qpreq.trace_id();
        std::string os;
        if(sess->qpreq.has_device() && sess->qpreq.device().has_os())
          os=sess->qpreq.device().os();
        LOG_INFO("`imei=%s`trace_id=%s`os=%s",sess->imei.c_str(),trace_id.c_str(),os.c_str());

        rt=ra->send_cmd(sess, "hget %s %s", "user_perspective", sess->imei.c_str());
        if(rt != 0)
        {
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }

        sess->status ^= FLAG_USER;

    }while(0);
    return rt;
}
int QpServer::send_get_user_game_tag(SessData * sess)
{
    int rt=0;
    do{
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(RA_USER_GAME);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        
        util::UtilStr ustr;
        ustr.format("hget user_game_perspective %s", sess->imei.c_str());
        LOG_DEBUG("cmd[%s]", ustr.str().c_str() );

        rt=ra->send_cmd(sess, "hget %s %s", "user_game_perspective", sess->imei.c_str());
        if(rt != 0)
        {
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }

        sess->status ^= FLAG_USER_GAME;

    }while(0);
    return rt;
}

int QpServer::send_get_device_price(SessData * sess)
{
    int rt=0;
    do{
        QPRequest & qpreq=sess->qpreq;
        std::string brand=qpreq.device().brand();
        std::string model=qpreq.device().model();
        std::string key=util::UtilStr::inst(brand).to_lower().str()+"`"+util::UtilStr::inst(model).to_lower().str();
        RedisAccess * ra=RedisPool::get_mutable_instance().get_redis_instance(RA_DEVICE);
        if(ra == NULL)
        {
            MON_ADD(ATTR_GET_REDIS_ERR, 1);
            LOG_ERROR("get_redis_instance error");
            rt=-1;
            break;
        }
        util::UtilStr ustr;
        ustr.format("hget device_price %s", key.c_str());
        LOG_DEBUG("cmd[%s]", ustr.str().c_str() );
        rt=ra->send_cmd(sess, "hget %s %s", "device_price", key.c_str() );
        if(rt != 0)
        {
            MON_ADD(ATTR_SEND_CMD_ERROR, 1);
            break;
        }

        sess->status ^= FLAG_DEVICE;

    }while(0);
    return rt;
}

int QpServer::on_get_user_tag(void * data, void * reply, int err_code, const char * err_str )
{
    int rt=0;
    SessData * sess=(SessData *)data;
    redisReply * pReply=(::redisReply *)reply;
    do{
        if(!sess_valid(sess))
        {
            MON_ADD(ATTR_SESS_INVALID, 1);
            LOG_ERROR("sess invalid[%d]", getpid());
            rt=-1;
            break;
        }
        if(err_code != 0)
        {
            LOG_ERROR("on_get_user_tag error[%d][%s]\n", err_code, err_str);
        }else if(pReply == NULL )
        {
            LOG_ERROR("pReply == NULL");
        }else if(pReply->type != REDIS_REPLY_STRING)
        {
            if(pReply->type == REDIS_REPLY_NIL)
            {
                MON_ADD(ATTR_USER_TAG_NO_KEY, 1);
            }
            if(pReply->type == REDIS_REPLY_ERROR)
            {
                LOG_ERROR("on_get_user_game_tag pReply->type != STRING[%d][%s]", pReply->type, pReply->str);
            }
            LOG_DEBUG("on_get_user_tag pReply->type != STRING[%d]", pReply->type);

        }else
        {
            MON_ADD(ATTR_USER_TAG_HAVE_DATA, 1);
            std::string value=pReply->str;
            std::vector<std::string> vrstr;
            boost::split(vrstr, value, boost::is_any_of("`"));
         
            int user_tag_id[]={TAGID_AGE,
                TAGID_GRENDER,
                TAGID_DEGREE,
                TAGID_CAREER,
                TAGID_COLLEGE_STUDENT,
                TAGID_MARRIAGE,
                TAGID_IDCARD_AGE,
                TAGID_USER_LEVEL};
         
            for(size_t i=0; i<vrstr.size() && i < 8; i++)
            {
                if(!vrstr[i].empty())
                {
                  LOG_DEBUG("user tag: [%d] [%s]",user_tag_id[i],vrstr[i].c_str());
                  sess->tags[user_tag_id[i]]=vrstr[i];
                }
            }
        }
        sess->status ^= FLAG_USER;
        rt=proc_sess(sess);
        if(rt != 0)
        {
            LOG_ERROR("proc_sess error");
            break;
        }
        
    }while(0);
    return rt;
    
}
int QpServer::on_get_user_game_tag(void * data, void * reply , int err_code, const char * err_str )
{
    int rt=0;
    SessData * sess=(SessData *)data;
    redisReply * pReply=(redisReply *)reply;
    do{
        if(!sess_valid(sess))
        {
            MON_ADD(ATTR_SESS_INVALID, 1);
            LOG_ERROR("sess invalid[%d]", getpid());
            rt=-1;
            break;
        }else if(err_code != 0)
        {
            LOG_ERROR("get_user_game_tag error[%d][%s]\n", err_code, err_str);
        }
        if(pReply == NULL )
        {
            LOG_ERROR("alloc pReply error");
        }
        if(pReply->type != REDIS_REPLY_STRING)
        {
            if(pReply->type == REDIS_REPLY_NIL)
            {
                MON_ADD(ATTR_USER_GAME_TAG_NO_KEY, 1);
            }
            if(pReply->type == REDIS_REPLY_ERROR)
            {
                LOG_ERROR("on_get_user_game_tag pReply->type != STRING[%d][%s]", pReply->type, pReply->str);
            }
            LOG_DEBUG("on_get_user_game_tag pReply->type != STRING[%d]", pReply->type);
        }else
        {
            MON_ADD(ATTR_USER_GAME_TAG_HAVE_DATA, 1);
            std::string value=pReply->str;
            std::vector<std::string> vrstr;
            boost::split(vrstr, value, boost::is_any_of("`"));
         
            int user_game_tag_id[]={
                TAGID_GAME_CATEGORIES,
                TAGID_GAME_THEME,
                TAGID_GAME_PLAY,
                TAGID_GAME_CULTURE,
                TAGID_GAME_FEATURE,
                TAGID_GAME_LEVEL,
                TAGID_PAY_USER,
                TAGID_LAST_LOGIN_GAME,
                TAGID_LAST_PAY_GAME,
                TAGID_FIRST_PAY_GAME,
            };
         
            for(size_t i=0; i<vrstr.size() && i<10; i++)
            {
                if(!vrstr[i].empty())
                {
                  LOG_DEBUG("user game tag: [%d] [%s]",user_game_tag_id[i],vrstr[i].c_str());
                  sess->tags[user_game_tag_id[i]]=vrstr[i];
                }
            }
        }
        sess->status ^= FLAG_USER_GAME;
        rt=proc_sess(sess);
        if(rt != 0)
        {
            LOG_ERROR("proc_sess error");
            break;
        }
        
        
    }while(0);
    return rt;
}

int QpServer::on_get_device_price(void * data, void * reply, int err_code, const char * err_str )
{
    int rt=0;
    SessData * sess=(SessData *)data;
    redisReply * pReply=(redisReply *)reply;
    do{
        if(!sess_valid(sess))
        {
            MON_ADD(ATTR_SESS_INVALID, 1);
            LOG_ERROR("sess invalid[%d]", getpid());
            rt=-1;
            break;
        }

        if(err_code != 0)
        {
            LOG_ERROR("on_get_device_pric error[%d][%s]\n", err_code, err_str);
        }else if(pReply == NULL )
        {
            LOG_ERROR("alloc redis Reply error");
        }else if(pReply->type != REDIS_REPLY_STRING)
        {
            if(pReply->type == REDIS_REPLY_NIL)
            {
                MON_ADD(ATTR_DEV_TAG_NO_KEY, 1);
            }
            if(pReply->type == REDIS_REPLY_ERROR)
            {
                LOG_ERROR("on_get_user_game_tag pReply->type != STRING[%d][%s]", pReply->type, pReply->str);
            }
            LOG_DEBUG("on_get_device_price pReply->type != STRING[%d]", pReply->type);
        }else
        {
            MON_ADD(ATTR_DEV_TAG_HAVE_DATA, 1);
            std::string value=pReply->str;
            std::vector<std::string> vrstr;
            boost::split(vrstr, value, boost::is_any_of("`"));

            int user_game_tag_id[]={
                TAGID_DEVICE_PRICE,
                TAGID_BRAND,
                TAGID_MODEL};


            for(size_t i=0; i<vrstr.size() && i < 3; i++)
            {
                if(!vrstr[i].empty())
                {
                  LOG_DEBUG("brand tag: [%d] [%s]",user_game_tag_id[i],vrstr[i].c_str());
                  sess->tags[user_game_tag_id[i]]=vrstr[i];
                }
            }
        }
        sess->status ^= FLAG_DEVICE;
        rt=proc_sess(sess);
        if(rt != 0)
        {
            LOG_ERROR("proc_sess error");
            break;
        }
        
        
    }while(0);
    return rt;
}

int QpServer::proc_sess(SessData * sess)
{
    int rt=0;
    do{
        if( sess->status != 0 )
        {
            break;
        }
        QPRequest & qpreq=sess->qpreq;
        QPResponse & qprsp=sess->qprsp;
        const rtb::Device & device =qpreq.device();

        DmpInfo * dmpinfo=qprsp.mutable_dmp_info();

#define ADD_USER_TAG(ID, VALUE) do{ \
    common::Targetting * tag=dmpinfo->add_user_targets();\
    tag->set_type(ID);\
    tag->add_value(VALUE);\
}while(0)
#define ADD_ORS_TAG(ID, VALUE) do{ \
    common::Targetting * tag=dmpinfo->add_ors_targets();\
    tag->set_type(ID);\
    tag->add_value(VALUE);\
}while(0)

#define ADD_USER_TAG_VEC(ID, VEC) do{ \
    common::Targetting * tag=dmpinfo->add_user_targets();\
    tag->set_type(ID);\
    size_t cnt=VEC.size();\
    for(size_t i=0; i<cnt; i++)\
    {\
        tag->add_value(VEC[i]);\
    }\
}while(0)

#define ADD_ORS_TAG_VEC(ID, VEC) do{ \
    common::Targetting * tag=dmpinfo->add_ors_targets();\
    tag->set_type(ID);\
    size_t cnt=VEC.size();\
    for(size_t i=0; i<cnt; i++)\
        tag->add_value(VEC[i]);\
}while(0)

#define ADD_USER_TAG_VEC_FROM_TAGS(ID) do{ \
    if(sess->tags.count(ID) > 0) \
    {\
        ADD_USER_TAG_VEC(ID, util::UtilStr::inst(sess->tags[ID]).split(","));\
    }\
}while(0)

#define ADD_ORS_TAG_VEC_FROM_TAGS(ID) do{ \
    if(sess->tags.count(ID) > 0) \
    {\
        ADD_ORS_TAG_VEC(ID, util::UtilStr::inst(sess->tags[ID]).split(","));\
    }\
}while(0)

#define ADD_USER_TAG_FROM_TAGS(ID) do { \
    if(sess->tags.count(ID) > 0) \
    {\
        ADD_USER_TAG(ID, sess->tags[ID]);\
    }\
}while(0)
#define ADD_ORS_TAG_FROM_TAGS(ID) do { \
    if(sess->tags.count((ID)) > 0) \
    {\
        ADD_ORS_TAG(ID, sess->tags[ID]);\
    }\
}while(0)
        
        if(device.has_os())
        {
            ADD_USER_TAG(TAGID_OS, DmpUtil::os_to_id(util::UtilStr::inst(device.os()).trim().to_upper().str()));//user
        }

        ADD_USER_TAG(TAGID_NETWORK, DmpUtil::connection_type_to_id(device.connection_type()));

        if(qpreq.has_site())
        {
            const rtb::Site & site=qpreq.site();
            size_t categories_size=site.page_categories_size();
            std::vector<std::string> vr; 
            for(size_t i=0; i < categories_size; i++)
            {
                int categ=site.page_categories(i);
                vr.push_back(util::Func::to_str(categ));
            }
            ADD_USER_TAG_VEC(TAGID_CONTENT_CATEGORIES, vr);
            ADD_USER_TAG_VEC(TAGID_CATEGORY, vr);


        }else if(qpreq.has_app())
        {
            const rtb::App & app=qpreq.app();
            size_t categories_size=app.page_categories_size();
            std::vector<std::string> vr; 
            for(size_t i=0; i < categories_size; i++)
            {
                int categ=app.page_categories(i);
                vr.push_back(util::Func::to_str(categ));
            }
            ADD_USER_TAG_VEC(TAGID_CONTENT_CATEGORIES, vr);
        }

//        ADD_USER_TAG(TAGID_CATEGORY);
#if 0
        ADD_USER_TAG(TAGID_LOCATION);
        ADD_USER_TAG(TAGID_CARRIER);
#endif

        ADD_USER_TAG_FROM_TAGS(TAGID_DEVICE_PRICE);
        ADD_USER_TAG_FROM_TAGS(TAGID_AGE);
        ADD_USER_TAG_FROM_TAGS(TAGID_GRENDER);
        ADD_USER_TAG_FROM_TAGS(TAGID_DEGREE);
        ADD_USER_TAG_FROM_TAGS(TAGID_CAREER);
        ADD_USER_TAG_FROM_TAGS(TAGID_COLLEGE_STUDENT);
        ADD_USER_TAG_FROM_TAGS(TAGID_MARRIAGE);

        ADD_USER_TAG_VEC_FROM_TAGS(TAGID_GAME_CATEGORIES);
        ADD_USER_TAG_VEC_FROM_TAGS(TAGID_GAME_THEME);
        ADD_USER_TAG_VEC_FROM_TAGS(TAGID_GAME_PLAY);
        ADD_USER_TAG_VEC_FROM_TAGS(TAGID_GAME_CULTURE);
        ADD_USER_TAG_FROM_TAGS(TAGID_PAY_USER);

        ADD_ORS_TAG_VEC_FROM_TAGS(TAGID_GAME_FEATURE);
        ADD_ORS_TAG_VEC_FROM_TAGS(TAGID_GAME_LEVEL);
        ADD_ORS_TAG_FROM_TAGS(TAGID_LAST_LOGIN_GAME);
        ADD_ORS_TAG_FROM_TAGS(TAGID_LAST_PAY_GAME);
        ADD_ORS_TAG_FROM_TAGS(TAGID_FIRST_PAY_GAME);

        ADD_ORS_TAG_FROM_TAGS(TAGID_BRAND);
        ADD_ORS_TAG_FROM_TAGS(TAGID_MODEL);

        ADD_ORS_TAG_FROM_TAGS(TAGID_IDCARD_AGE);
        ADD_ORS_TAG_FROM_TAGS(TAGID_USER_LEVEL);

#undef ADD_USER_TAG_FROM_TAGS
#undef ADD_ORS_TAG_FROM_TAGS
#undef ADD_ORS_TAG_VEC_FROM_TAGS
#undef ADD_USER_TAG_VEC_FROM_TAGS
#undef ADD_ORS_TAG_VEC
#undef ADD_USER_TAG_VEC
#undef ADD_USER_TAG
#undef ADD_ORS_TAG
        qprsp.set_error_code(common::ERROR_NONE);
        reply_client(sess);

    }while(0);
    return rt;
}

int QpServer::s_on_get_user_tag(void * data, void * reply, int err_code, const char * err_str )
{
    return QpServer::get_mutable_instance().on_get_user_tag(data, reply, err_code, err_str);
}

int QpServer::s_on_get_user_game_tag(void * data, void * reply , int err_code, const char * err_str)
{
    return QpServer::get_mutable_instance().on_get_user_game_tag(data, reply, err_code, err_str);
}
int QpServer::s_on_get_device_price(void * data, void * reply, int err_code, const char * err_str )
{
    return QpServer::get_mutable_instance().on_get_device_price(data, reply, err_code, err_str);
}

}
}


