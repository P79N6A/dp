NEED_PROTO=1
NEED_BOOST=1
NEED_LOG=1
NEED_HIREDIS=1
CXX=g++
CXXFLAGS+=-g
LDFLAGS+=-lpthread
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
#BINS=$(patsubst %.cpp, %, $(FILES))
TARG=bidding_exploration 
all:$(TARG)

$(TARG):$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	rm -f $(OBJS)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS) $(OBJS) $(TARG)
include ../../../Makefile.inc
