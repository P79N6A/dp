#include "zk4cpp.h"
#include <string.h>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <sys/select.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <inttypes.h>
#include <stddef.h>



Zk4Cpp::Zk4Cpp()
{
	zk_handle_ = NULL;
	errcode_ = 0;
}


Zk4Cpp::~Zk4Cpp()
{
	disconnect();
}


int Zk4Cpp::init(string const& strHost)
{
	zk_iplist_ = strHost;
	zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
	return 0;
}


void Zk4Cpp::watcher(zhandle_t *t, int type, int state, const char *path, void* context)
{
	Zk4Cpp* adapter=(Zk4Cpp*)context;
	adapter->onEvent(t, type, state, path);
}

void Zk4Cpp::onEvent(zhandle_t *, int type, int state, const char *path)
{
    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE){
            return onConnect();
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            return onExpired();
        } else if (state == ZOO_CONNECTING_STATE){
            return onConnecting();
        } else if (state == ZOO_ASSOCIATING_STATE){
            return onAssociating();
        }
    }else if (type == ZOO_CREATED_EVENT){
        return onNodeCreated(state, path);
    }else if (type == ZOO_DELETED_EVENT){
        return onNodeDeleted(state, path);
    }else if (type == ZOO_CHANGED_EVENT){
        return onNodeChanged(state, path);
    }else if (type == ZOO_CHILD_EVENT){
        return onNodeChildChanged(state, path);
    }else if (type == ZOO_NOTWATCHING_EVENT){
        return onNoWatching(state, path);
    }
    onOtherEvent(type, state, path);

}


int Zk4Cpp::connect(const clientid_t *clientid, int flags, watcher_fn watch, void *context)
{
	// Clear the connection state
	disconnect();

	errcode_ = 0;

	// Establish a new connection to ZooKeeper
	zk_handle_ = zookeeper_init(zk_iplist_.c_str(), 
        watch?watch:Zk4Cpp::watcher, 
		2000,
		clientid,
        watch?context:this,
		flags);

	if (zk_handle_ == NULL)
	{

//		snprintf(errmsg_, 2047, "Unable to connect to ZK running at '%s'", zk_iplist_.c_str());
		return -1;
	}
	return 0;
}


void Zk4Cpp::disconnect()
{
	if (zk_handle_ != NULL)
	{
		zookeeper_close (zk_handle_);
		zk_handle_ = NULL;
	}
}


int Zk4Cpp::createNode(const string &path, 
						   char const* data,
						   uint32_t dataLen,
						   const struct ACL_vector *acl,
						   int flags,
                           bool recursive,
						   char* pathBuf,
						   uint32_t pathBufLen)
{
	const int MAX_PATH_LENGTH = 2048;
	char realPath[MAX_PATH_LENGTH];
	realPath[0] = 0;
	int rc;
	errcode_ = 0;

	if (!isConnected())
	{
        errmsg_="No connect";
		return -1;
	}

	if (!pathBuf)
	{
		pathBuf = realPath;
		pathBufLen = MAX_PATH_LENGTH;
	}
    rc = zoo_create( zk_handle_, 
        path.c_str(), 
        data,
        dataLen,
        acl?acl:&ZOO_OPEN_ACL_UNSAFE,
        flags,
        pathBuf,
        pathBufLen);
    if (recursive && (ZNONODE == rc)){
        for (string::size_type pos = 1; pos != string::npos; ){
            pos = path.find( "/", pos );
            if (pos != string::npos){
                if (-1 == (rc = createNode(path.substr( 0, pos ), NULL, 0, acl, 0, true, pathBuf, pathBufLen))){
                    return -1;
                }
                pos++;
            }else{
                // No more path components
                return createNode(path, data, dataLen, acl, 0, false, pathBuf, pathBufLen);
            }
        }
    }
	if (rc != ZOK) // check return status
	{
		errcode_ = rc;
		if (ZNODEEXISTS == rc) return 0;
//		snprintf(errmsg_, 2047, "Error in creating ZK node [%s], err:%s err-code:%d.", path.c_str(), zerror(rc), rc);
		return -1;
	}
	return 1;
}


int Zk4Cpp::deleteNode(const string &path,
						   bool recursive,
						   int version)
{
	errcode_ = 0;
//	memset(errmsg_, 0x00, sizeof(errmsg_));
	
	if (!isConnected())
	{
        errmsg_="No connect";
        
		return -1;
	}

	int rc;
	rc = zoo_delete( zk_handle_, path.c_str(), version);

	if (rc != ZOK) //check return status
	{
		errcode_ = rc;
		if (ZNONODE == rc) return 0;
		if ((rc == ZNOTEMPTY) && recursive)
		{
			list<string> childs;
			if (!getNodeChildren(path, childs)) return false;
			string strPath;
			list<string>::iterator iter=childs.begin();
			while(iter != childs.end())
			{
				strPath = path + "/" + *iter;
				if (!deleteNode(strPath, true)) return false;
				iter++;
			}
			return deleteNode(path);
		}
//		snprintf(errmsg_, 2047, "Unable to delete zk node [%s], err:%s  err-code=%d", path.c_str(), zerror(rc), rc);
		return -1;
	}
	return 1;
}


int Zk4Cpp::getNodeChildren( const string &path, list<string>& childs, int watch)
{
	errcode_ = 0;
//	memset(errmsg_, 0x00, sizeof(errmsg_));

	if (!isConnected())
	{
		errmsg_="No connect";
		return -1;
	}

	String_vector children;
	memset( &children, 0, sizeof(children) );
	int rc;
	rc = zoo_get_children( zk_handle_,
		path.c_str(), 
		watch,
		&children );

	if (rc != ZOK) // check return code
	{
		errcode_ = rc;
		if (rc == ZNONODE) return 0;
//		snprintf(errmsg_, 2047, "Failure to get node [%s] child, err:%s err-code:%d", path.c_str(), zerror(rc), rc);
		return -1;
	}
	childs.clear();
	for (int i = 0; i < children.count; ++i)
	{
		childs.push_back(string(children.data[i]));
	}

    if (children.data) {
        int32_t i;
        for (i=0; i<children.count; i++) {
            free(children.data[i]);
        }
        free(children.data);
    }
	return 1;
}

int Zk4Cpp::nodeExists(const string &path, struct Stat& stat, int watch)
{
	errcode_ = 0;
//	memset(errmsg_, 0x00, sizeof(errmsg_));

	if (!isConnected())
	{
//		strcpy(errmsg_, "No connect");
		return -1;
	}

	memset(&stat, 0, sizeof(stat) );
	int rc;
	rc = zoo_exists( zk_handle_,
		path.c_str(),
		watch,
		&stat);
	if (rc != ZOK)
	{
		if (rc == ZNONODE) return 0;
		errcode_ = rc;
//		snprintf(errmsg_, 2047, "Error in checking existance of [%s], err:%s err-code:%d", path.c_str(), zerror(rc), rc);
		return -1;
	}
	return 1;
}

int Zk4Cpp::getNodeData(const string &path, char* data, uint32_t& dataLen, struct Stat& stat, int watch)
{
	errcode_ = 0;
//	memset(errmsg_, 0x00, sizeof(errmsg_));

	if (!isConnected())
	{
//		strcpy(errmsg_, "No connect");
		return -1;
	}

	memset(&stat, 0, sizeof(stat) );

	int rc = 0;
	int len = dataLen;
	rc = zoo_get( zk_handle_,
		path.c_str(),
		watch,
		data,
		&len,
		&stat);
	if (rc != ZOK) // checl return code
	{
		errcode_ = rc;
		if (rc == ZNONODE) return 0;
//		snprintf(errmsg_, 2047, "Error in fetching value of [%s], err:%s err-code:%d", path.c_str(), zerror(rc), rc);
		return -1;
	}
	dataLen = len;
	data[len] = 0x00;
	return 1;
}



int Zk4Cpp::wgetNodeChildren( const string &path, list<string>& childs, watcher_fn watcher, void* watcherCtx)
{
    errcode_ = 0;
//    memset(errmsg_, 0x00, sizeof(errmsg_));

    if (!isConnected())
    {
//        strcpy(errmsg_, "No connect");
        return -1;
    }

    String_vector children;
    memset( &children, 0, sizeof(children) );
    int rc;
    rc = zoo_wget_children( zk_handle_,
        path.c_str(), 
        watcher,
        watcherCtx,
        &children );

    if (rc != ZOK) // check return code
    {
        errcode_ = rc;
        if (rc == ZNONODE) return 0;
//        snprintf(errmsg_, 2047, "Failure to get node [%s] child, err:%s err-code:%d", path.c_str(), zerror(rc), rc);
        return -1;
    }
    childs.clear();
    for (int i = 0; i < children.count; ++i)
    {
        childs.push_back(string(children.data[i]));
    }
    return 1;

}

int Zk4Cpp::wnodeExists(const string &path, struct Stat& stat, watcher_fn watcher, void* watcherCtx)
{
    errcode_ = 0;
//    memset(errmsg_, 0x00, sizeof(errmsg_));

    if (!isConnected())
    {
//        strcpy(errmsg_, "No connect");
        return -1;
    }

    memset(&stat, 0, sizeof(stat) );
    int rc;
    rc = zoo_wexists( zk_handle_,
        path.c_str(),
        watcher,
        watcherCtx,
        &stat);
    if (rc != ZOK)
    {
        if (rc == ZNONODE) return 0;
        errcode_ = rc;
//        snprintf(errmsg_, 2047, "Error in checking existance of [%s], err:%s err-code:%d", path.c_str(), zerror(rc), rc);
        return -1;
    }
    return 1;

}

int Zk4Cpp::wgetNodeData(const string &path, char* data, uint32_t& dataLen, struct Stat& stat, watcher_fn watcher, void* watcherCtx)
{
    errcode_ = 0;

    if (!isConnected())
    {
        errmsg_="No connect";
        return -1;
    }

    memset(&stat, 0, sizeof(stat) );

    int rc = 0;
    int len = dataLen;
    rc = zoo_wget( zk_handle_,
        path.c_str(),
        watcher,
        watcherCtx,
        data,
        &len,
        &stat);
    if (rc != ZOK) // checl return code
    {
        errcode_ = rc;
        if (rc == ZNONODE) return 0;
//        snprintf(errmsg_, 2047, "Error in fetching value of [%s], err:%s err-code:%d", path.c_str(), zerror(rc), rc);
        return -1;
    }
    dataLen = len;
    data[len] = 0x00;
    return 1;
}



int Zk4Cpp::setNodeData(const string &path, char const* data, uint32_t dataLen, int version)
{
	errcode_ = 0;
//	memset(errmsg_, 0x00, sizeof(errmsg_));

	if (!isConnected())
	{
//		strcpy(errmsg_, "No connect");
		return -1;
	}

	int rc;
	rc = zoo_set( zk_handle_,
		path.c_str(),
		data,
		dataLen,
		version);

	if (rc != ZOK) // check return code
	{
		errcode_ = rc;
		if (rc == ZNONODE) return 0;
		
//		snprintf(errmsg_, 2047, "Error in set value of [%s], err:%s err-code:%d", path.c_str(), zerror(rc), rc);
		return -1;
	}
	// success
	return 1;
}



void Zk4Cpp::sleep(uint32_t miliSecond)
{
	struct timeval tv;
	tv.tv_sec = miliSecond/1000;
	tv.tv_usec = (miliSecond%1000)*1000;
	select(1, NULL, NULL, NULL, &tv);
}


int Zk4Cpp::split(string const& src, list<string>& value, char ch)
{
	string::size_type begin = 0;
	string::size_type end = src.find(ch);
	int num=1;
	value.clear();
	while(string::npos != end)
	{
		value.push_back(src.substr(begin, end - begin));
		begin = end + 1;
		end = src.find(ch, begin);
		num++;
	}	
	value.push_back(src.substr(begin));	
	return num;
}


static char const* toString(int64_t llNum, char* szBuf, int base)
{
	char const* szFormat=(16==base)?"%""llx":"%""lld";
	sprintf(szBuf, szFormat, llNum);
	return szBuf;
}

void Zk4Cpp::dumpStat(struct Stat const& stat, string& info)
{
	char szTmp[64];
	char line[1024];
	time_t timestamp;
	snprintf(line, 1024, "czxid:%s\n", toString(stat.czxid, szTmp, 16));
	info = line;
	
	snprintf(line, 1024, "mzxid:%s\n", toString(stat.mzxid, szTmp, 16));
	info += line;
	
	timestamp = stat.ctime/1000;
	snprintf(line, 1024, "ctime:%d %s", (int)(stat.ctime%1000), ctime_r(&timestamp, szTmp));
	info += line;
	
	timestamp = stat.mtime/1000;
	snprintf(line, 1024, "mtime:%d %s", (int)(stat.mtime%1000), ctime_r(&timestamp, szTmp));
	info += line;

	snprintf(line, 1024, "version:%d\n", stat.version);
	info += line;

	snprintf(line, 1024, "cversion:%d\n", stat.cversion);
	info += line;

	snprintf(line, 1024, "aversion:%d\n", stat.aversion);
	info += line;

	snprintf(line, 1024, "dataLength:%d\n", stat.dataLength);
	info += line;

	snprintf(line, 1024, "numChildren:%d\n", stat.numChildren);
	info += line;

	snprintf(line, 1024, "pzxid:%s\n", toString(stat.pzxid, szTmp, 16));
	info += line;
}

#define R2LOG_DBG(fmt, a...) do{}while(0)
#ifndef R2LOG_DBG
#define R2LOG_DBG(fmt, a...) fprintf(stderr, "[%d in %s]DEBUG:"fmt, __LINE__, __FILE__, ##a)
#endif

void Zk4Cpp::noop_string_completion(int rc, const char *value, const void *data)
{
    R2LOG_DBG("rc[%d], value[%s]\n", rc, value );
    return;
}

void Zk4Cpp::noop_stat_completion(int rc, const struct Stat *stat, const void *data)
{
    R2LOG_DBG("rc[%d]\n", rc);
    return;
}

void Zk4Cpp::noop_void_completion(int rc, const void *data)
{
    R2LOG_DBG("rc[%d]\n", rc);
    return;
}

void Zk4Cpp::create_node_completion(int rc, const char *value, const void *data)
{
    R2LOG_DBG("rc[%d]\n", rc);
    if(data==NULL)
    {
        return;
    }
    CallBackInfo * pCallBackInfo=(CallBackInfo *)data;
    Zk4Cpp * pZk=(Zk4Cpp *)(pCallBackInfo->pAddr);
    if(pZk==NULL)
    {
        return;
    }
    pZk->OnCreateNodeAsync(pCallBackInfo->path,rc);
    delete pCallBackInfo;

    return;
}

void Zk4Cpp::set_data_completion(int rc, const struct Stat *stat, const void *data)
{
    R2LOG_DBG("rc[%d]\n", rc);
    if(data == NULL)
    {
        return;
    }
    CallBackInfo * pCallBackInfo=(CallBackInfo *)data;
    Zk4Cpp * pZk=(Zk4Cpp *)(pCallBackInfo->pAddr);
    if(pZk==NULL)
    {
        return;
    }
    pZk->onSetDataAsync(pCallBackInfo->path, rc);

    delete pCallBackInfo;

    return;
}

void Zk4Cpp::delete_node_completion(int rc, const void *data)
{
    R2LOG_DBG("rc[%d]\n", rc);
    if(data == NULL)
    {
        return;
    }
    CallBackInfo * pCallBackInfo=(CallBackInfo *)data;
    Zk4Cpp * pZk=(Zk4Cpp *)(pCallBackInfo->pAddr);
    if(pZk==NULL)
    {
        return;
    }
    pZk->OnDeleteNodeAsync(pCallBackInfo->path, rc);

    delete pCallBackInfo;
    return;
}

int Zk4Cpp::createNodeAsync(const string &path, 
    char const* data,
    uint32_t dataLen,
    const struct ACL_vector *acl,
    int flags)
{
	int rc;
	errcode_ = 0;
//	memset(errmsg_, 0x00, sizeof(errmsg_));

	if (!isConnected())
	{
//		strcpy(errmsg_, "No connect");
		return -1;
	}

    CallBackInfo * pCallBackInfo=new(std::nothrow) CallBackInfo();
    if(pCallBackInfo == NULL)
    {
        return -1;
    }
    pCallBackInfo->pAddr=this;
    pCallBackInfo->path=path;

    rc = zoo_acreate( zk_handle_, 
        path.c_str(), 
        data,
        dataLen,
        acl?acl:&ZOO_OPEN_ACL_UNSAFE,
        flags,
        create_node_completion,
        (void * )pCallBackInfo);
	if (rc != ZOK) // check return status
	{
		errcode_ = rc;
		if (ZNODEEXISTS == rc) return 0;
//		snprintf(errmsg_, 2047, "Error in creating ZK node [%s], err:%s err-code:%d.", path.c_str(), zerror(rc), rc);
		return -1;
	}
	return 1;
}

int Zk4Cpp::createNodeAsync(const string &path, 
            char const* data,
            uint32_t dataLen,
            string_completion_t completion,
            void * context=NULL,
            const struct ACL_vector *acl,
            int flags)
{
	int rc;
	errcode_ = 0;
//	memset(errmsg_, 0x00, sizeof(errmsg_));

	if (!isConnected())
	{
//		strcpy(errmsg_, "No connect");
		return -1;
	}

    rc = zoo_acreate( zk_handle_, 
        path.c_str(), 
        data,
        dataLen,
        acl?acl:&ZOO_OPEN_ACL_UNSAFE,
        flags,
        completion,
        context);
	if (rc != ZOK) // check return status
	{
		errcode_ = rc;
		if (ZNODEEXISTS == rc) return 0;
//		snprintf(errmsg_, 2047, "Error in creating ZK node [%s], err:%s err-code:%d.", path.c_str(), zerror(rc), rc);
		return -1;
	}
	return 1;
}

int Zk4Cpp::setNodeDataAsync(const string &path, char const* data, uint32_t dataLen, int version)
{
	errcode_ = 0;
//	memset(errmsg_, 0x00, sizeof(errmsg_));

	if (!isConnected())
	{
//		strcpy(errmsg_, "No connect");
		return -1;
	}

    CallBackInfo * pCallBackInfo=new(std::nothrow) CallBackInfo();
    if(pCallBackInfo == NULL)
    {
        return -1;
    }
    pCallBackInfo->pAddr=this;
    pCallBackInfo->path=path;

	int rc;
	rc = zoo_aset( zk_handle_,
		path.c_str(),
		data,
		dataLen,
		version,
        set_data_completion,
        (void *)pCallBackInfo);

	if (rc != ZOK) // check return code
	{
		errcode_ = rc;
		if (rc == ZNONODE) return 0;
		
//		snprintf(errmsg_, 2047, "Error in set value of [%s], err:%s err-code:%d", path.c_str(), zerror(rc), rc);
		return -1;
	}
	// success
	return 1;
}

int Zk4Cpp::setNodeDataAsync(const string &path,
        char const* data,
        uint32_t dataLen,
        stat_completion_t completion,
        void * context,
        int version)
{
	errcode_ = 0;

	if (!isConnected())
	{
//		strcpy(errmsg_, "No connect");
		return -1;
	}


	int rc;
	rc = zoo_aset( zk_handle_,
		path.c_str(),
		data,
		dataLen,
		version,
        completion,
        context);

	if (rc != ZOK) // check return code
	{
		errcode_ = rc;
		if (rc == ZNONODE) return 0;
		
//		snprintf(errmsg_, 2047, "Error in set value of [%s], err:%s err-code:%d", path.c_str(), zerror(rc), rc);
		return -1;
	}
	// success
	return 1;
}

int Zk4Cpp::deleteNodeAsync(const string &path, int version)
{
	errcode_ = 0;
	
	if (!isConnected())
	{
//		strcpy(errmsg_, "No connect");
		return -1;
	}
    CallBackInfo * pCallBackInfo=new(std::nothrow) CallBackInfo();
    if(pCallBackInfo == NULL)
    {
        return -1;
    }
    pCallBackInfo->pAddr=this;
    pCallBackInfo->path=path;

	int rc;
	rc = zoo_adelete( zk_handle_, path.c_str(), version, delete_node_completion,
            (void *)pCallBackInfo);

	if (rc != ZOK) //check return status
	{
		errcode_ = rc;
		if (ZNONODE == rc) return 0;
//		snprintf(errmsg_, 2047, "Unable to delete zk node [%s], err:%s  err-code=%d", path.c_str(), zerror(rc), rc);
		return -1;
	}
	return 1;
}

int Zk4Cpp::deleteNodeAsync(const string &path,
        void_completion_t completion,
        void * context,
        int version)
{
	errcode_ = 0;
	
	if (!isConnected())
	{
//		strcpy(errmsg_, "No connect");
		return -1;
	}

	int rc;
	rc = zoo_adelete( zk_handle_, path.c_str(), version, completion,
            context);

	if (rc != ZOK) //check return status
	{
		errcode_ = rc;
		if (ZNONODE == rc) return 0;
//		snprintf(errmsg_, 2047, "Unable to delete zk node [%s], err:%s  err-code=%d", path.c_str(), zerror(rc), rc);
		return -1;
	}
	return 1;
}

///}}}

void Zk4Cpp::onNodeCreated(int , char const* )
{
}

void Zk4Cpp::onNodeDeleted(int , char const* )
{
}

void Zk4Cpp::onNodeChanged(int , char const* )
{
}

void Zk4Cpp::onNodeChildChanged(int , char const* )
{
}

void Zk4Cpp::onNoWatching(int , char const* )
{
}	

void Zk4Cpp::onOtherEvent(int , int , const char *)
{
}
