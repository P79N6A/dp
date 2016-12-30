#pragma once

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "boost/serialization/singleton.hpp"
#include <boost/algorithm/string.hpp> 
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
#include "evhttp.h"
#include "util/timer.h"
#include "util/log.h"
#include "util/util_str.h"
#include "util/timeout_event.h"
#include "util/redis_client.h"
#include <sstream> 

using namespace std;

#define BULKET_INDEX_LEN 2