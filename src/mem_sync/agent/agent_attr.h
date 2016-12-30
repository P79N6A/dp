/*
 * monitor.h
 * Created on: 2016-11-07
 */

#ifndef SRC_MEM_SYNC_AGENT_AGENT_ATTR_H_
#define SRC_MEM_SYNC_AGENT_AGENT_ATTR_H_

#define ATTR_DATA_AGENT_ZK_LOST          2243	/* zookeeper lost */
#define ATTR_DATA_AGENT_ZK_CONNECT	     2250	/* zookeeper connect */
#define ATTR_DATA_AGENT_UPDATE_FAIL      2244   /* mem_manager update fail */
#define ATTR_DATA_AGENT_MD5_FAIL  	     2245	/* md5 fail */
#define ATTR_DATA_AGENT_DATA_TRANSMITTED 2251   /* how much data is transmitted in KB */
#define ATTR_DATA_AGENT_VER_NOT_MATCH    2252   /* version not match alarm */

#endif /* SRC_MEM_SYNC_AGENT_AGENT_ATTR_H_ */
