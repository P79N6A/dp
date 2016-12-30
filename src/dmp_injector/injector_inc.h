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
#include "mysql.h" 

using namespace std;

#define DEVICE_PRICE_TAG_REDIS_KEY  "qp_device_price_tags"