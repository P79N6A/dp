NEED_GFLAGS=1
NEED_EXP=1
NEED_MEM_SYNC=1
NEED_MEM_KV=1
NEED_BOOST=1
NEED_UTIL=1
NEED_LOG=1
NEED_PROTOBUF=1
NEED_PROTO=1
CXX=g++
CXXFLAGS+=-g
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
BINS=$(patsubst %.cpp, %, $(FILES))
LDFLAGS+=../../src/sn_new/inverted_index/libinverted_index.a
all:$(BINS)

$(BINS):%:%.o
	$(CXX) -o $@ $< $(LDFLAGS)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS) $(OBJS)
include ../../Makefile.inc
