#ifndef _ZK_ADAPTOR_H_
#define _ZK_ADAPTOR_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <inttypes.h>

using namespace std;

extern "C" {
#include "zookeeper.h"
}

class Zk4Cpp
{
public:

	Zk4Cpp();

	virtual ~Zk4Cpp(); 

	int init(string const& strHost);

	virtual int connect(const clientid_t *clientid=NULL, int flags=0, watcher_fn watch=NULL, void *context=NULL);

	void disconnect();


	bool isConnected()
	{
		if (zk_handle_){
			int state = zoo_state (zk_handle_);
			if (state == ZOO_CONNECTED_STATE) return true;
		} 
		return false;
	}

	int createNode(const string &path, 
		char const* data,
		uint32_t dataLen,
		const struct ACL_vector *acl=&ZOO_OPEN_ACL_UNSAFE,
		int flags=0,
        bool recursive=false,
		char* pathBuf=NULL,
		uint32_t pathBufLen=0);

	int deleteNode(const string &path,
		bool recursive = false,
		int version = -1);

	int getNodeChildren( const string &path, list<string>& childs, int watch=0);

	int nodeExists(const string &path, struct Stat& stat, int watch=0);

	int getNodeData(const string &path, char* data, uint32_t& dataLen, struct Stat& stat, int watch=0);

    int wgetNodeChildren( const string &path, list<string>& childs, watcher_fn watcher=NULL, void* watcherCtx=NULL);
    
    int wnodeExists(const string &path, struct Stat& stat, watcher_fn watcher=NULL, void* watcherCtx=NULL);
    
    int wgetNodeData(const string &path, char* data, uint32_t& dataLen, struct Stat& stat, watcher_fn watcher=NULL, void* watcherCtx=NULL);

	int setNodeData(const string &path, char const* data, uint32_t dataLen, int version = -1);


	string const& getHost() const { return zk_iplist_;}

	zhandle_t* getZkHandle() { return zk_handle_;}

	const clientid_t * getClientId() { return  isConnected()?zoo_client_id(zk_handle_):NULL;}

	int  getErrCode() const { return errcode_;}

	char const* getErrMsg() const { return errmsg_.c_str();}

    typedef struct
    {
        Zk4Cpp * pAddr;
        std::string path;
    }CallBackInfo;

    /**
     * @brief               noop's callback function
     */
    static void noop_string_completion(int rc, const char *value, const void *data);

    /**
     * @brief               noop's callback function
     */
    static void noop_stat_completion(int rc, const struct Stat *stat, const void *data);

    /**
     * @brief               noop's callback function
     */
    static void noop_void_completion(int rc, const void *data);
    
    /**
     * @brief               createNodeAsync's callback function
     */
    static void create_node_completion(int rc, const char *value, const void *data);

    /**
     * @brief               setNodeDataASync's callback function
     */
    static void set_data_completion(int rc, const struct Stat *stat, const void *data);

    /**
     * @brief               deleteNodeAsync's callback function
     */
    static void delete_node_completion(int rc, const void *data);

	int createNodeAsync(const string &path, 
		char const* data,
		uint32_t dataLen,
		const struct ACL_vector *acl=&ZOO_OPEN_ACL_UNSAFE,
		int flags=0);

	int createNodeAsync(const string &path, 
		char const* data,
		uint32_t dataLen,
        string_completion_t completion,
        void * context,
		const struct ACL_vector *acl=&ZOO_OPEN_ACL_UNSAFE,
		int flags=0);
	
	int setNodeDataAsync(const string &path,
            char const* data,
            uint32_t dataLen,
            int version = -1);
    
	int setNodeDataAsync(const string &path,
            char const* data,
            uint32_t dataLen,
            stat_completion_t completion,
            void * context,
            int version = -1);

	int deleteNodeAsync(const string &path, int version = -1);

	int deleteNodeAsync(const string &path,
            void_completion_t completion,
            void * context,
            int version = -1);
    
    virtual void onSetDataAsync(const string & path, int rc){};

    virtual void OnCreateNodeAsync(const string & path, int rc){};

    virtual void OnDeleteNodeAsync(const string & path, int rc){};

///}}}

public:
	
	static void sleep(uint32_t miliSecond);

	static int split(string const& src, list<string>& value, char ch);


	static void dumpStat(struct Stat const& stat, string& info);
public:
    virtual void onEvent(zhandle_t *t, int type, int state, const char *path);

    virtual void onConnect(){}

    virtual void onAssociating(){}

    virtual void onConnecting(){}

    virtual void onExpired(){}

    virtual void onNodeCreated(int state, char const* path);

    virtual void onNodeDeleted(int state, char const* path);

    virtual void onNodeChanged(int state, char const* path);

    virtual void onNodeChildChanged(int state, char const* path);

    virtual void onNoWatching(int state, char const* path);

    virtual void onOtherEvent(int type, int state, const char *path);

private:
	static void watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);

private:
	string       zk_iplist_;
	zhandle_t*   zk_handle_;
	int          errcode_;
    std::string  errmsg_;
};

#endif /* _ZK_ADAPTER_H_ */
