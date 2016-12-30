#make install 仅copy文件到bin目录
#make install push=true 打包并推送到正式环境UAE
#make install push_pre=true 打包并推送到预发布环境UAE
# 正式发布的appid
APPID=2871
#预发布的appid
APPID_PRE=2857
#目标二进制文件名
TARG=poseidon_adapter
#需要编译的公共模块
COMMON_DIRS=protocol util src/ha src/zk4cpp
#当前目录位置
MY_DIR=src/adapter
#在envs需要打包的文件夹位置
PUSH_DIR=poseidon_adapter

NEED_BOOST=1
NEED_MUDUO=1
NEED_PROTO=1
NEED_SSL=1
NEED_MYSQL=1
NEED_LOG=1
NEED_HA=1
NEED_ZK4CPP=1
NEED_MONITOR_NEW=1
NEED_ZLIB=1
NEED_GFLAGS=1
NEED_HIREDIS=1

include ../../Makefile.inc

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


src=$(shell find ./ -name "*.cpp")
obj=$(patsubst %.cpp, %.o, $(src))
CXXFLAGS+=-std=c++0x -Wno-old-style-cast

LDFLAGS+=-pthread

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

$(TARG):$(obj)
	$(CXX) -o $@ $^ $(LDFLAGS)
	rm -f $(ROOT_DIR)/envs/poseidon/bin/$(TARG)
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/


%.o:%.cpp
	$(CXX) $(CXXFLAGS) -DASYNLOG -c -o $@ $^

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
	@-rm $(TARG);find ./ -name "*.o"|xargs rm


#all:$(TARG)

install:
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
	cp -r $(TARG) $(ROOT_DIR)/envs/$(PUSH_DIR)/bin/
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



