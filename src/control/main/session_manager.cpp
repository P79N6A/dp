
#include "session_manager.h"
#include "util/log.h"
#include "util/monitor.h"

#include "scheduler.h"
#include "src/comm_event/comm_event_factory.h"
#include "src/comm_event/comm_event_interface.h"
#include "src/comm_event/comm_event_timer.h"

namespace poseidon
{
namespace control
{

/**
 * @brief       be called on session timeout
 * @return      success-0, or other errcode
 **/
int Session::on_timeout()
{
    int rt=0;
    do{
        MON("session.timeout", 1);
        status_=STAT_TIME_OUT;
        dispatch();

    }while(0);
    return rt;
}


/**
 * @brief       be called on session be alloced
 * @return      success-0, or other errcode
 **/
int Session::on_alloc()
{
    int rt=0;
    do{
        data().prog=0;
        data().enabled_backup_ad=false;
    }while(0);
    return rt;
}


/**
 * @brief       be called on session be freeed
 * @return      success-0, or other errcode
 **/
int Session::on_free()
{
    int rt=0;
    do{
        /*TODO:add pvlog here*/
    }while(0);
    return rt;
}

/**
 * @brief       free self
 **/
void Session::free()
{
    if(manager_!=NULL && sid_ >= 0)
    {
        manager_->free_session(sid_);
    }
}

int Session::dispatch()
{
    return Scheduler::get_mutable_instance().dispatch(this);
}

/**
 * @brief   capacity of SessionManager
 **/
int SessionManager::init(int max_capacity)
{
    if(!init_)
    {
        max_capacity_=max_capacity;
        session_pool_.reserve(max_capacity);
        session_pool_.resize(max_capacity_, NULL);
        //use resize replace for
#if 0
        for(int i=0; i<max_capacity_;i++)
        {
            session_pool_[i]=NULL;
        }
#endif
        init_=true;
        return true;
    }else
    {
        LOG_WARN("SessionManager have inited");
        return false;
    }
}

/**
 * @brief  set session timeout(ms)
 **/
void SessionManager::set_session_timeout(int ms)
{
    time_out_=ms;
}


/**
 * @brief  alloc session
 * @return  return session id, fail return -1
 **/
int SessionManager::alloc_session()
{
    MON("session.alloc", 1);
    int rt=0;
    do{
        int old_index=alloc_index_;
        while(session_pool_[alloc_index_]!=NULL)
        {
            alloc_index_=(alloc_index_+1)%max_capacity_;
            if(alloc_index_==old_index)
            {
                LOG_ERROR("session pool full!");
                rt=-1;
                break;
            }
        }
        if(rt < 0)
        {
            break;
        }
        session_pool_[alloc_index_]=new Session();
        Session * sess=session_pool_[alloc_index_];
        sess->set_sid(alloc_index_);
        sess->set_manager(this);
        sess->set_timeout(time_out_);
        sess->on_alloc();
        dc::common::comm_event::CommFactoryInterface::instance().add_comm_timer(sess);
        rt=alloc_index_;
        alloc_index_=(alloc_index_+1)%max_capacity_;
    }while(0);
    return rt;
}


/**
 * @brief       get session from session id
 * @param sid   [IN], SID
 * @return      return the session, if session is invalid,return NULL
 **/
Session * SessionManager::get_session(int sid)
{
    Session * rt=NULL;
    do{
        if(sid > max_capacity_)
        {
            rt=NULL;
            break;
        }
        if(session_pool_[sid] == NULL)
        {
            break;
        }
        rt=session_pool_[sid];
    }while(0);
    return rt;
}


/**
 * @brief       free session
 * @param sid   [IN], session ID 
 **/
void SessionManager::free_session(int sid)
{
    MON("session.free", 1);
    if(session_pool_[sid] != NULL)
    {
        session_pool_[sid]->on_free();
        delete session_pool_[sid];
        session_pool_[sid]=NULL;
    }
}

}
}


#ifdef __TEST_SESSION__

int main(int argc, char * argv[])
{
    int rt=0;
    do{
        SessionManager::get_mutable_instance().init(10000);
        SessionManager::get_mutable_instance().set_session_timeout(100);
        sid=SessionManager::get_mutable_instance().alloc_session();
        if(sid < 0)
        {
            LOG_ERROR("error");
            break;
        }
        SessionData * pSess=SessionManager::get_mutable_instance().get_session(sid);
        if(pSess == NULL)
        {
            LOG_ERROR("error");
            break;
        }
        pSess->reqpack="I am a reqpack";
        pSess->free();
    }while(0);
    return rt;
}

#endif
