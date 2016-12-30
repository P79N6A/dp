NEED_BOOST=1
NEED_LOG=1
NEED_ZK4CPP=1
NEED_UTIL=1
NEED_MEM_SYNC=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
TARG=libposeidon_mem_sync.a

all:$(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	$(AR) -rv $@ $^ 

clean:
	rm  -f $(OBJS) $(TARG)

include ../../../Makefile.inc

