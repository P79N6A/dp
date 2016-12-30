
#make install 仅copy文件到bin目录
#make install push=true 打包并推送到正式环境UAE
#make install push_pre=true 打包并推送到预发布环境UAE

# 正式发布的appid
APPID=2910
#预发布的appid
APPID_PRE=2856
#目标二进制文件名
TARG=poseidon_sn
#需要编译的公共模块
COMMON_DIRS=protocol util src/comm_event src/monitor  src/zk4cpp src/ha src/exp_sys/api src/sn_new/inverted_index src/scoring src/mem_sync src/model_updater/api

#在envs需要打包的文件夹位置
PUSH_DIR=poseidon_sn

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

NEED_COMM_EVENT=1
NEED_BOOST=1
NEED_LOG=1
NEED_PROTO=1
NEED_PROTOBUF=1
NEED_JSONCPP=1
NEED_HA=1
NEED_ZK4CPP=1
NEED_MONITOR_NEW=1
NEED_SSL=1
NEED_GFLAGS=1
NEED_MEM_SYNC=1
NEED_MEM_KV=1
NEED_EXP=1
NEED_MODEL_UPDATER=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
PROJ_DIR=../../..

LDFLAGS+=../inverted_index/libinverted_index.a

CXXFLAGS+= -I$(PROJ_DIR)/src/scoring
LDFLAGS+=$(PROJ_DIR)/src/scoring/api/libscoringapi.a
LDFLAGS+=$(PROJ_DIR)/src/exp_sys/api/libposeidon_exp.a
LDFLAGS+=$(ROOT_DIR)/src/model_updater/api/libmodelupdaterapi.a

#NO_O2=1
#CXXFLAGS+=-DFOR_UNIT_TEST

all:
	@cd $(PROJ_DIR);\
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


$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	rm -f $(ROOT_DIR)/envs/poseidon/bin/$(TARG)
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/

	#clean 包括公共模块的内容
clean:
	@cd $(PROJ_DIR) ;\
	for x in $(COMMON_DIRS); \
	do \
	make -C $$x $@; \
	if [ $$? != 0 ]; then \
		exit 1; \
	fi \
	done
	rm  -f $(OBJS) $(TARG)
	
install:
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
	tar -cvzf $(ROOT_DIR)/envs/poseidon/bin/$(TARG).tar.gz $(ROOT_DIR)/envs/poseidon/bin/$(TARG)
	md5sum $(ROOT_DIR)/envs/poseidon/bin/$(TARG).tar.gz
	cp -r $(TARG) $(ROOT_DIR)/envs/$(PUSH_DIR)/bin/ 
	cp -r $(TARG) $(ROOT_DIR)/pre_envs/$(PUSH_DIR)/bin/ 
	cp -r $(TARG) $(ROOT_DIR)/pre_envs/$(PUSH_DIR)/bin/$(TARG)_pre
	
	@if [ '$(PUSH)' == 'true' ];then  \
		version=`./$(TARG) --version | awk '{print $$3}'`;\
		echo "sh $(ROOT_DIR)/push/push_uae.sh $(PUSH_DIR) $(APPID) $${version} envs";\
		sh $(ROOT_DIR)/push/push_uae.sh $(PUSH_DIR) $(APPID) $${version} envs;  \
	fi
	
	@if [ '$(PUSH_PRE)' == 'true' ];then  \
		version=`./$(TARG) --version | awk '{print $$3}'`;\
		echo "sh $(ROOT_DIR)/push/push_uae.sh $(PUSH_DIR) $(APPID_PRE) $${version} pre_envs";\
		sh $(ROOT_DIR)/push/push_uae.sh $(PUSH_DIR) $(APPID_PRE) $${version} pre_envs;  \
	fi

include $(PROJ_DIR)/Makefile.inc
