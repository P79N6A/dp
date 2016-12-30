CXX=g++
LIBEVENT_DIR=$(ROOT_DIR)/third_party/libevent
HIREDIS_DIR=$(ROOT_DIR)/third_party/hiredis
CXXFLAGS=-g -I$(ROOT_DIR) -I$(HIREDIS_DIR)/include  -I$(ROOT_DIR)/src/comm_event -I$(LIBEVENT_DIR)/include -g -Wall
LDFLAGS=-L$(ROOT_DIR)/src/comm_event -L$(ROOT_DIR)/protocol/lib  -lcomm_event $(LIBEVENT_DIR)/lib/libevent_core.a $(HIREDIS_DIR)/lib/libhiredis.a  -lrt
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
BINS=$(patsubst %.cpp, %, $(FILES))
NEED_PROTO=1
all:$(BINS)

$(BINS):%:%.o
	$(CXX) -o $@ $< $(LDFLAGS)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS) $(OBJS)

include ../../Makefile.inc
