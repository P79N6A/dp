#pragma once

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "boost/serialization/singleton.hpp"
#include <boost/algorithm/string.hpp> 
#include "boost/lockfree/queue.hpp"
#include "protocol/src/poseidon_dmp_tag_data.pb.h"
#include "json/json.h"
#include "gflags/gflags.h"
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <boost/atomic.hpp>
#include <dirent.h>
#include <queue>
#include "util/func.h"
#include "util/mutex.h"
#include "util/thread_pool.h"
#include <boost/shared_ptr.hpp>
#include <hiredis.h>
#include <async.h>
#include <adapters/libevent.h>
#include "event.h"
#include "util/timer.h"
#include "ha.h"
#include "util/log.h"
#include "util/util_str.h"
#include "util/timeout_event.h"
#include "monitor_api.h"
#include <sstream> 

using namespace std;

#define UDP_SERV_RECV_BUFFER_SIZE 1024*128
#define UDP_SERV_SEND_BUFFER_SIZE 1024*128
#define BULKET_INDEX_LEN 2
#define DEVICE_PRICE_TAG_REDIS_KEY  "qp_device_price_tags"

/*
 #define QP_TAGID_GRENDER		1001
 #define QP_TAGID_AGE		1002
 #define QP_TAGID_IDCARD_AGE		1008
 #define QP_TAGID_DEGREE		1003
 #define QP_TAGID_CAREER		1004
 #define QP_TAGID_COLLEGE_STUDENT		1005
 #define QP_TAGID_MARRIAGE		1006
 */
#define QP_TAGID_LOCATION		1007
#define QP_TAGID_DEVICE_PRICE		2001
#define QP_TAGID_OS		2002
#define QP_TAGID_BRAND		2005
#define QP_TAGID_MODEL		2006
#define QP_TAGID_CARRIER		2003
#define QP_TAGID_NETWORK		2004
#define QP_TAGID_CONTENT_CATEGORIES		3001
#define QP_TAGID_APP_CATEGORIES		3002
#define QP_TAGID_CATEGORY		3003
/*
 #define QP_TAGID_PAY_USER		4001
 #define QP_TAGID_GAME_CATEGORIES		4002
 #define QP_TAGID_GAME_THEME		4003
 #define QP_TAGID_GAME_PLAY		4004
 #define QP_TAGID_GAME_CULTURE		4005
 #define QP_TAGID_GAME_FEATURE		4006
 #define QP_TAGID_GAME_LEVEL		4007
 #define QP_TAGID_LAST_LOGIN_GAME		4008
 #define QP_TAGID_LAST_PAY_GAME		4009
 #define QP_TAGID_FIRST_PAY_GAME		4010
 #define QP_TAGID_USER_LEVEL		5001
 #define QP_TAGID_SEED_USER    5002
 */
