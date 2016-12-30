NEED_BOOST = 1
NEED_UTIL = 1
NEED_LOG = 1
NEED_SSL = 1
NEED_PROTO = 1
NEED_COMM_EVENT = 1
NEED_ZK4CPP = 1

SHELL = /bin/bash
TARG = datasvr
OBJS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))

CXXFLAGS += -I../
all: $(TARG)

$(TARG): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	
$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<
	
clean:
	rm -rf $(OBJS) $(TARG)


include ../../../Makefile.inc
