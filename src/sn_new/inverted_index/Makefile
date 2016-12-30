NEED_BOOST=1
NEED_LOG=1
NEED_UTIL=1
NEED_MEM_SYNC=1
NEED_MEM_KV=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))

TARG=libinverted_index.a

all:$(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm  -f $(OBJS) $(TARG)

$(TARG):$(OBJS)
	$(AR) -rv $(TARG) $(OBJS)

include ../../../Makefile.inc

