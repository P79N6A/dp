NEED_PROTO=1
NEED_JSONCPP=1
NEED_BOOST=1
NEED_LOG=1
NEED_SSL=1

CXX=g++
CXXFLAGS+=-g
LDFLAGS+=-lpthread
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
#BINS=$(patsubst %.cpp, %, $(FILES))
TARG=cxr_stats_parser 
all:$(TARG)

$(TARG):$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS) $(OBJS) $(TARG)
include ../../../../Makefile.inc
