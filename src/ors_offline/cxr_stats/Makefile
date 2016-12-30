#make install 仅copy文件到bin目录
#make install push=true 打包并推送到正式环境UAE
#make install push_pre=true 打包并推送到预发布环境UAE

# 正式发布的appid
APPID=3042
#预发布的appid
APPID_PRE=3043
#目标2禁止文件名
TARG=cxr_stats
#需要编译的公共模块
COMMON_DIRS=protocol util

NEED_PROTO=1
NEED_BOOST=1
NEED_LOG=1
NEED_GFLAGS=1
NEED_SSL=1


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
	#@cd ../../../ ;\
	#for x in $(COMMON_DIRS); \
	#do \
	#make -C $$x $@; \
	#if [ $$? != 0 ]; then \
	#	exit 1; \
	#fi \
	#done
	rm  -f $(OBJS) $(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	rm -f $(ROOT_DIR)/envs/poseidon/bin/$(TARG)
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
	md5sum $(TARG)


install:
	mkdir -p $(ROOT_DIR)/envs/$(TARG)/bin $(ROOT_DIR)/pre_envs/$(TARG)/bin
	mkdir -p $(ROOT_DIR)/envs/$(TARG)/log $(ROOT_DIR)/pre_envs/$(TARG)/log
	mkdir -p $(ROOT_DIR)/envs/$(TARG)/data $(ROOT_DIR)/pre_envs/$(TARG)/data
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
	cp -r $(TARG) $(ROOT_DIR)/envs/$(TARG)/bin/
	cp -r etc $(ROOT_DIR)/envs/$(TARG)/
	cp -r $(TARG) $(ROOT_DIR)/pre_envs/$(TARG)/bin/$(TARG)_pre
	cp -r etc $(ROOT_DIR)/pre_envs/$(TARG)/

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

