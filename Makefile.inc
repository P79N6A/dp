ROOT_DIR=$(shell \
	I=0; \
	DIR=".."; \
	while true;\
	do \
		if [ -f "$${DIR}/__POSEIDON__ROOT_DIR__" ]; then \
			echo "$${DIR}"; \
			break; \
		fi; \
		DIR="$${DIR}/.."; \
		let I=$$I+1; \
		if [ $$I -gt 10 ]; then \
			echo "can't find the root, please check __POSEIDON__ROOT_DIR__ file"; \
			break; \
		fi; \
	done\
)

PROTOBUF_LIB=$(shell \
		if [ -f "/usr/local/lib/libprotobuf.a" ]; then \
			echo "/usr/local/lib/libprotobuf.a"; \
		else \
			echo "$${MY_PROTOBUF_LIB}"; \
                fi; \
)

PROTOBUF_INC=$(shell \
		if [ -d "/usr/local/include/google/protobuf" ]; then \
			echo "/usr/local/include"; \
		else \
			echo "$${MY_PROTOBUF_INCLUDE}"; \
                fi; \
)

BUILD_INFO=build_in_$(shell date +%Y%m%d_%H_%M_%S )_by_$(shell whoami)

ifndef NO_O2
CXXFLAGS+=-O2 -Wall -fPIC -pipe -g -I. -I$(ROOT_DIR) -D 'BUILD_INFO="$(BUILD_INFO)"' -I/usr/local/include
else
CXXFLAGS+=-Wall -fPIC -pipe -g -I. -I$(ROOT_DIR) -D 'BUILD_INFO="$(BUILD_INFO)"' -I/usr/local/include
endif


ifdef NEED_EXP
CXXFLAGS+= -I$(ROOT_DIR)/src/exp_sys
LDFLAGS+=$(ROOT_DIR)/src/exp_sys/api/libposeidon_exp.a
endif

ifdef NEED_MODEL_UPDATER
CXXFLAGS+= -I$(ROOT_DIR)/src/model_updater/api
LDFLAGS+=$(ROOT_DIR)/src/model_updater/api/libmodelupdaterapi.a
endif

ifdef NEED_MEM_SYNC
CXXFLAGS+= -I$(ROOT_DIR)/src/mem_sync -I$(ROOT_DIR)/src/mem_sync/mem_manager
LDFLAGS+=$(ROOT_DIR)/src/mem_sync/data_api/libposeidon_mem_sync.a $(ROOT_DIR)/src/mem_sync/mem_manager/libmem_sync_mm.a
endif

ifdef NEED_MEM_KV
CXXFLAGS+= -I$(ROOT_DIR)/src/mem_sync
LDFLAGS+=$(ROOT_DIR)/src/mem_sync/mem_kv/libposeidon_mem_kv.a 
endif

ifdef NEED_JSONCPP
CXXFLAGS+=-I$(ROOT_DIR)/third_party/jsoncpp/include
LDFLAGS+=-L$(ROOT_DIR)/third_party/jsoncpp/lib/ -ljson_libmt
endif

ifdef NEED_HA
CXXFLAGS+=-I$(ROOT_DIR)/src/ha
LDFLAGS+=-L$(ROOT_DIR)/src/ha -lposeidon_ha
endif

ifdef NEED_ZK4CPP
CXXFLAGS+=-I$(ROOT_DIR)/src/zk4cpp -I$(ROOT_DIR)/third_party/zookeeper/include/zookeeper
LDFLAGS+=-L$(ROOT_DIR)/src/zk4cpp -lzk4cpp $(ROOT_DIR)/third_party/zookeeper/lib/libzookeeper_mt.a
endif

ifdef NEED_MONITOR_NEW
CXXFLAGS+=-I$(ROOT_DIR)/util -I$(ROOT_DIR)/src/monitor/api

LDFLAGS+= -L$(ROOT_DIR)/src/monitor/api -lmonitor_api -L$(ROOT_DIR)/util -lutil 
endif

ifdef NEED_LOG
CXXFLAGS+=-I$(ROOT_DIR)/util -I$(ROOT_DIR)/third_party/log4cpp/include
LDFLAGS+=-L$(ROOT_DIR)/util -lutil $(ROOT_DIR)/third_party/log4cpp/lib/liblog4cpp.a -pthread $(ROOT_DIR)/third_party/cityhash/lib/libcityhash.a
endif

ifdef NEED_MONITOR
CXXFLAGS+=-I$(ROOT_DIR)/util -I$(ROOT_DIR)/third_party/log4cpp/include
LDFLAGS+=-L$(ROOT_DIR)/util -lutil $(ROOT_DIR)/third_party/log4cpp/lib/liblog4cpp.a -pthread
endif


ifdef NEED_COMM_EVENT
CXXFLAGS+= -I$(ROOT_DIR)/src/comm_event -I$(ROOT_DIR)/third_party/libevent/include
LDFLAGS+=-L$(ROOT_DIR)/src/comm_event -lcomm_event $(ROOT_DIR)/third_party/libevent/lib/libevent_core.a -lrt
CXXFLAGS+=-I$(ROOT_DIR)/third_party/hiredis/include
LDFLAGS+=$(ROOT_DIR)/third_party/hiredis/lib/libhiredis.a
endif

ifdef NEED_BOOST
CXXFLAGS+=-I$(ROOT_DIR)/third_party/boost/include
LDFLAGS+=$(ROOT_DIR)/third_party/boost/lib/libboost_thread.a $(ROOT_DIR)/third_party/boost/lib/libboost_system.a
endif

ifdef NEED_MUDUO
CXXFLAGS+=-I$(ROOT_DIR)/third_party/muduo
LDFLAGS+=-L$(ROOT_DIR)/third_party/muduo/lib -lnet -lbase
endif



ifdef NEED_SSL
CXXFLAGS+=-I$(ROOT_DIR)/third_party/openssl/include
#LDFLAGS+=-L$(ROOT_DIR)/third_party/openssl/lib -lcrypto
LDFLAGS+=$(ROOT_DIR)/third_party/openssl/lib/libcrypto.a
endif

ifdef NEED_MYSQL
CXXFLAGS+=-I$(ROOT_DIR)/third_party/mysql/include
LDFLAGS+=-L$(ROOT_DIR)/third_party/mysql/lib -lmysqlcppconn-static -L$(ROOT_DIR)/third_party/lib $(ROOT_DIR)/third_party/mysql/lib/libmysqlclient_r.a -ldl -lrt
endif

ifdef NEED_PROTO
CXXFLAGS+=-I$(ROOT_DIR)/protocol/src -I${PROTOBUF_INC}
LDFLAGS+=-L$(ROOT_DIR)/protocol/lib -lposeidonproto ${PROTOBUF_LIB}
endif

ifdef NEED_PROTOBUF
CXXFLAGS+=-I${PROTOBUF_INC}
LDFLAGS+=${PROTOBUF_LIB}
endif


ifdef NEED_CITYHASH
LDFLAGS+=-L$(ROOT_DIR)/third_party/cityhash/lib -lcityhash
endif

ifdef NEED_HIREDIS
CXXFLAGS+=-I$(ROOT_DIR)/third_party/hiredis/include
LDFLAGS+=$(ROOT_DIR)/third_party/hiredis/lib/libhiredis.a
endif

ifdef NEED_ZLIB
CXXFLAGS+=-I$(ROOT_DIR)/third_party/zlib/include
LDFLAGS+=$(ROOT_DIR)/third_party/zlib/lib/libz.a
endif

ifdef NEED_GFLAGS
CXXFLAGS+=-I$(ROOT_DIR)/third_party/google/gflags/include
LDFLAGS+=$(ROOT_DIR)/third_party/google/gflags/lib/libgflags.a
endif

ifdef NEED_THREAD_POOL
CXXFLAGS+=-I$(ROOT_DIR)/third_party/thread_pool/
endif

ifdef NEED_MYSQL_C
CXXFLAGS+=-I$(ROOT_DIR)/third_party/mysql_c/include
LDFLAGS+=$(ROOT_DIR)/third_party/mysql_c/lib/libmysqlclient.a 
endif

ifdef NEED_LIB_EVENT
CXXFLAGS+= -I$(ROOT_DIR)/third_party/libevent/include
LDFLAGS+=$(ROOT_DIR)/third_party/libevent/lib/libevent.a
endif

ifdef NEED_JEMELLOC
CXXFLAGS+= -I$(ROOT_DIR)/third_party/jemelloc/include
LDFLAGS+=$(ROOT_DIR)/third_party/jemelloc/lib/libjemalloc.a
endif

ifdef NEED_COROUTINE
CXXFLAGS +=-I$(ROOT_DIR)/src/coroutine
LDFLAGS  += $(ROOT_DIR)/src/coroutine/co2/libco2.a
LDFLAGS  += $(ROOT_DIR)/src/coroutine/st/libst.a
LDFLAGS  += -ldl
endif

LDFLAGS+= -lrt -lz

CXX=g++
CC=gcc
AR=ar

