#make install 仅copy文件到bin目录
#make install push=true 打包并推送到正式环境UAE
#make install push_pre=true 打包并推送到预发布环境UAE

SHELL=/bin/bash

# 正式发布的appid
APPID=2913
#预发布的appid
APPID_PRE=2912
#目标2禁止文件名
TARG=dataagent
#需要编译的公共模块
COMMON_DIRS=protocol util src/zk4cpp src/mem_sync/data_api src/mem_sync/mem_manager src/monitor/api

NEED_JSONCPP=1
NEED_MEM_SYNC=1
NEED_SSL=1
NEED_BOOST=1
NEED_PROTO=1
NEED_UTIL=1
NEED_LOG=1
NEED_ZK4CPP=1
NEED_COMM_EVENT=1
NEED_MONITOR_NEW=1

FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))

ifdef push
PUSH=true
else
PUSH=false
endif

ifdef push_pre
PUSH_PRE=true
else
PUSH_PRE=false
endif

all:
	@cd ../../../ ;\
	for x in $(COMMON_DIRS); \
	do \
	make -C $$x $@; \
	if [ $$? != 0 ]; then \
		exit 1; \
	fi \
	done
	make alone

# 只编译当前目录
alone:$(TARG)

#clean 包括公共模块的内容
clean:
	# @cd ../../../ ;\
	# for x in $(COMMON_DIRS); \
	# do \
	# make -C $$x $@; \
	# if [ $$? != 0 ]; then \
	# 	exit 1; \
	# fi \
	# done
	rm  -f $(OBJS) $(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) -I ../mem_manager -I ../../monitor/api -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	$(CXX) -I ../mem_manager -o $@ $^ $(LDFLAGS)
	rm -f $(ROOT_DIR)/envs/poseidon/bin/$(TARG)
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
	md5sum $(TARG)


install:
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
	cp -r $(TARG) $(ROOT_DIR)/envs/$(TARG)/bin/
	cp -r $(TARG) $(ROOT_DIR)/pre_envs/$(TARG)/bin/$(TARG)_pre

	@if [ '$(PUSH)' == 'true' ];then  \
		version=`./$(TARG) --version | awk '{print $$3}'`;\
		echo "sh $(ROOT_DIR)/push/push_uae.sh $(TARG) $(APPID) $${version} envs";\
		sh $(ROOT_DIR)/push/push_uae.sh $(TARG) $(APPID) $${version} envs;  \
	fi

	@if [ '$(PUSH_PRE)' == 'true' ];then  \
		version=`./$(TARG) --version | awk '{print $$3}'`;\
		echo "sh $(ROOT_DIR)/uae/push_uae.sh $(TARG) $(APPID_PRE) $${version} pre_envs";\
		sh $(ROOT_DIR)/uae/push_uae.sh $(TARG) $(APPID_PRE) $${version} pre_envs;  \
	fi

include ../../../Makefile.inc

