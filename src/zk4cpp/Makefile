NEED_SSL=1
CXX=g++
CXXFLAGS=-I. -I$(ROOT_DIR)/third_party/zookeeper/include/zookeeper -I/usr/local/commlib/baselib
CXXFLAGS+=$(CXXFLAG)

FILES=$(wildcard ./*.cpp)
OBJS=$(patsubst %.cpp,%.o,$(FILES))
OBJS_NOATTR=$(patsubst %.cpp,%_noattr.o,$(FILES))

TARG=libzk4cpp.a

all:$(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ 

libzk4cpp.a:$(OBJS)
	ar -rv $@ $^

clean:
	rm -f $(OBJS)  $(TARG)

include ../../Makefile.inc
