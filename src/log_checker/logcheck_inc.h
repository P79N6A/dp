#pragma once

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "boost/serialization/singleton.hpp"
#include <boost/algorithm/string.hpp> 
#include "boost/lockfree/queue.hpp"
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
#include "event.h"
#include "util/timer.h"
#include "util/log.h"
#include "util/util_str.h"
#include "util/timeout_event.h"
#include "monitor_api.h"
#include <sstream> 

using namespace std;

#define UDP_SERV_RECV_BUFFER_SIZE           1024*128
#define LOG_CHECK_ERROR_MULTI_KEY           2324
#define LOG_CHECK_ERROR_NULL_VALUE          2323
#define LOG_CHECK_ERROR_NO_SOURCE           2325
#define LOG_CHECK_ERROR_ERROR_SOURCE        2326
#define LOG_CHECK_ERROR_NO_PID              2327
#define LOG_CHECK_STAT_TOO_MANY_PID         2328
#define LOG_CHECK_STAT_YOUTU_LESS_PARAM     2329
#define LOG_CHECK_STAT_IQIYI_LESS_PARAM     2330
#define LOG_CHECK_STAT_TOUTIAO_LESS_PARAM   2331
#define LOG_CHECK_STAT_UNKNOW_OS            2355
#define LOG_CHECK_STAT_IOS_REQ              2356
#define LOG_CHECK_STAT_IOS_NO_DEV_ID        2357
#define LOG_CHECK_STAT_ANDROID_REQ          2358

