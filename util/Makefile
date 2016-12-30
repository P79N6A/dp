NEED_LIB_EVENT=1
NEED_COMM_EVENT=1
NEED_BOOST=1
NEED_LOG=1
NEED_SSL=1
NEED_CITYHASH=1
NEED_THREAD_POOL=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
TARG=libutil.a
CXXFLAGS+=-I$(ROOT_DIR)/third_party
all:$(TARG)


$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	ar -rv $@ $^

clean:
	rm  -f $(OBJS) $(TARG)

include ../Makefile.inc

