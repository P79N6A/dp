NEED_LOG=1
NEED_BOOST=1
NEED_PROTO=1
NEED_HA=1
NEED_ZK4CPP=1
NEED_UTIL=1
NEED_COMM_EVENT=1
CXX=g++
CXXFLAGS+=-g
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
BINS=$(patsubst %.cpp, %, $(FILES))
all:$(BINS)

city_hash:city_hash.o
	$(CXX) -o $@ $^ $(LDFLAGS)

property_look:$(PROTO_OBJ) property_look.o
	$(CXX) -o $@ $^ $(LDFLAGS)

dump_ors_req:$(PROTO_OBJ) dump_ors_req.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test_client_ors:$(PROTO_OBJ) test_client_ors.o
	$(CXX) -o $@ $^ $(LDFLAGS)

main:main.o
	$(CXX) -o $@ $^ $(LDFLAGS)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS) $(OBJS)
include ../../../Makefile.inc
