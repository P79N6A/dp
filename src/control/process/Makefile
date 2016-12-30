NEED_EXP=1
NEED_MEM_SYNC=1
NEED_COMM_EVENT=1
NEED_BOOST=1
NEED_LOG=1
NEED_HA=1
NEED_ZK4CPP=1
NEED_MONITOR_NEW=1
NEED_PROTO=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
TARG=libcontrolprocess.a
all:$(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	ar -rv $@ $^

clean:
	rm  -f $(OBJS) $(TARG)

include ../../../Makefile.inc

