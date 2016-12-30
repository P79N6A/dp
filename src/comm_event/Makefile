CXX=g++
#LIBEVENT_DIR=/usr/local/libevent
HIREDIS_DIR=../../third_party/hiredis
LIBEVENT_DIR=../../third_party/libevent
CXXFLAGS= -I$(LIBEVENT_DIR)/include -I$(HIREDIS_DIR)/include  -g -Wall
LDFLAGS=$(LIBEVENT_DIR)/lib/libevent_core.a -lrt
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
BINS=$(patsubst %.cpp, %, $(FILES))
TARG=libcomm_event.a

all:$(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<  

$(TARG):$(OBJS)
	ar -rv $@ $^

clean:
	rm -f $(OBJS) $(TARG)
