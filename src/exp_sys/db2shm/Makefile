#make install 仅copy文件到bin目录
#make install push=true 打包并推送到正式环境UAE
#make install push_pre=true 打包并推送到预发布环境UAE

#目标二进制文件名
TARG=poseidon_exp_db2shm
#需要编译的公共模块
COMMON_DIRS=protocol util src/zk4cpp src/exp_sys/api src/mem_sync/data_api src/mem_sync/mem_manager
#当前目录位置
MY_DIR=src/exp/db2shm

NEED_PROTO=1
NEED_MYSQL_C=1
NEED_BOOST=1
NEED_LOG=1
NEED_ZK4CPP=1
NEED_JSONCPP=1
NEED_SSL=1
NEED_GFLAGS=1
NEED_THREAD_POOL=1
NEED_MEM_SYNC=1
NEED_EXP=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))

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


$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	rm -f $(ROOT_DIR)/envs/poseidon/bin/$(TARG)
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/

	#clean 包括公共模块的内容
clean:
#	@cd ../../../ ;\

#	for x in $(COMMON_DIRS); \
	do \
	make -C $$x $@; \
	if [ $$? != 0 ]; then \
		exit 1; \
	fi \
	done
	rm  -f $(OBJS) $(TARG)

include ../../../Makefile.inc





