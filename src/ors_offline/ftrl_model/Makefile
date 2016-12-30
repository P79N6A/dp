NEED_PROTO=1
NEED_LOG=1
NEED_GFLAGS=1
NEED_BOOST=1

CXX=g++
CXXFLAGS+=-g
LDFLAGS+=-L../data_model -lorsofflinedatamodel
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
TARG=ftrl_model
all:$(TARG)

$(TARG):$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	rm -f $(OBJS)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS) $(OBJS) $(TARG)
include ../../../Makefile.inc
