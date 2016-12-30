NEED_BOOST=1
NEED_LOG=1
NEED_UTIL=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
TARG=libmonitor_api.a

LDFLAGS+=-lcrypto

all:$(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	$(AR) -rv $@ $^ 

clean:
	rm  -f $(OBJS) $(TARG)

install:
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
include ../../../Makefile.inc

