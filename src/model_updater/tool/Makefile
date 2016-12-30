NEED_LOG=1
NEED_BOOST=1
NEED_PROTO=1
NEED_HA=1
NEED_ZK4CPP=1
NEED_UTIL=1
NEED_COMM_EVENT=1
NEED_MEM_SYNC=1
NEED_MEM_KV=1

CXX=g++
CXXFLAGS+=-g
LDFLAGS+=-L../api -lmodelupdaterapi
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
BINS=$(patsubst %.cpp, %, $(FILES))
all:$(BINS)

main:main.o
	$(CXX) -o $@ $^ $(LDFLAGS)


dump_model_data:dump_model_data.o
	    $(CXX) -o $@ $^ $(LDFLAGS)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS) $(OBJS)
include ../../../Makefile.inc
