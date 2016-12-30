SHELL=$$(which bash)

NEED_COROUTINE=1
NEED_BOOST=1
NEED_UTIL = 1
NEED_LOG=1
NEED_SSL=1
NEED_PROTO=1
NEED_MONITOR_NEW=1
NEED_HIREDIS=1
NEED_HA=1
NEED_ZK4CPP=1

FILES=$(wildcard *.cpp redis/*.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
TARG=poseidon_fb

CXXFLAGS += -std=c++0x -I../

all: $(TARG)
	
$(TARG):$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	
$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<
	
clean:
	rm -rf $(OBJS) $(TARG)

include ../../Makefile.inc

