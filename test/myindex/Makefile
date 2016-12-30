CXX=g++
LIBEVENT_DIR=$(ROOT_DIR)/third_party/libevent
CXXFLAGS=-g -I$(ROOT_DIR)  -I$(ROOT_DIR)/src/comm_event -I$(LIBEVENT_DIR)/include -g -Wall
LDFLAGS=-L$(ROOT_DIR)/src/comm_event -L$(ROOT_DIR)/protocol/lib -lposeidonproto /usr/local/lib/libprotobuf.a -lcomm_event $(LIBEVENT_DIR)/lib/libevent_core.a -lrt
PROTO_FILE=$(wildcard *.proto)
PROTO_SRC=$(patsubst %.proto, %.pb.cc, $(PROTO_FILE))
PROTO_OBJ=$(patsubst %.proto, %.pb.o, $(PROTO_FILE))
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
BIN=myindex_server test_client
NEED_PROTO=1
NEED_BOOST=1
all:$(BIN)

$(PROTO_SRC):%.pb.cc:%.proto
	protoc -I. -I/usr/local/include $< --cpp_out=./ 

$(PROTO_OBJ):%.o:%.cc
	$(CXX) -c $(CXXFLAGS) -o $@ $<

myindex_server:$(PROTO_OBJ) myindex_server.o myindex.o
	$(CXX) -o $@ $^ $(LDFLAGS)

test_client:$(PROTO_OBJ) test_client.o 
	$(CXX) -o $@ $^ $(LDFLAGS)


$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(BINS) $(OBJS) $(PROTO_OBJ)

include ../../Makefile.inc
