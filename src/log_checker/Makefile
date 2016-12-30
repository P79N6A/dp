#make install 仅copy文件到bin目录
#make install push=true 打包并推送到正式环境UAE
#make install push_pre=true 打包并推送到预发布环境UAE

# 正式发布的appid
APPID=3026
#预发布的appid
#APPID_PRE=2854
#目标2禁止文件名
TARG=pvlog_checker
#需要编译的公共模块
COMMON_DIRS=util src/comm_event src/monitor 
#当前目录位置
MY_DIR=src/log_checker
#在envs需要打包的文件夹位置
PUSH_DIR=log_checker
NEED_PROTO=1
NEED_COMM_EVENT=1
NEED_LIB_EVENT=1
NEED_BOOST=1
NEED_LOG=1
NEED_JSONCPP=1
NEED_SSL=1
NEED_HIREDIS=1
NEED_MONITOR_NEW=1
NEED_GFLAGS=1
NEED_THREAD_POOL=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))

ifdef push
PUSH=true
else
PUSH=false
endif

#ifdef push_pre
#PUSH_PRE=true
#else
#PUSH_PRE=false
#endif

all:
	@cd ../../ ;\
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
	@cd ../../ ;\
	for x in $(COMMON_DIRS); \
	do \
	make -C $$x $@; \
	if [ $$? != 0 ]; then \
		exit 1; \
	fi \
	done
	rm  -f $(OBJS) $(TARG)
	
$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	rm -f $(ROOT_DIR)/envs/poseidon/bin/$(TARG)
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
	md5sum $(TARG)


install:
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
	cp -r $(TARG) $(ROOT_DIR)/envs/$(PUSH_DIR)/bin/ 
	
	@if [ '$(PUSH)' == 'true' ];then  \
		version=`./$(TARG) --version | awk '{print $$3}'`;\
		echo "sh $(ROOT_DIR)/uae/push_uae.sh $(PUSH_DIR) $(APPID) $${version} envs";\
		sh $(ROOT_DIR)/uae/push_uae.sh $(PUSH_DIR) $(APPID) $${version} envs;  \
	fi

include ../../Makefile.inc


