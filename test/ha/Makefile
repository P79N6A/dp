NEED_LOG=1
NEED_BOOST=1
NEED_HA=1
NEED_ZK4CPP=1
NEED_UTIL=1
CXX=g++
CXXFLAGS+=-g
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
BINS=$(patsubst %.cpp, %, $(FILES))
all:$(BINS)

$(BINS):%:%.o
	$(CXX) -o $@ $< $(LDFLAGS)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS) $(OBJS)
include ../../Makefile.inc
