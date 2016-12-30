SHELL = /bin/bash
TARG = libco2.a

COMMON_OBJ = $(wildcard *.cpp)
BUFFER_OBJ = $(wildcard buffer/*.cpp)
SERVER_OBJ = $(wildcard server/*.cpp)
CLIENT_OBJ = $(wildcard client/*.cpp)
SYNC_OBJ   = $(wildcard sync/*.cpp)
STREAM_OBJ = $(wildcard stream/*.cpp)
CLIENT_OBJ = $(wildcard client/*.cpp)
TIMER_OBJ  = $(wildcard timer/*.cpp)
TEST_OBJ = $(wildcard test/*.cpp)

OBJS = $(patsubst %.cpp, %.o, $(COMMON_OBJ) $(BUFFER_OBJ) $(CLIENT_OBJ) $(SERVER_OBJ) $(STREAM_OBJ) $(TIMER_OBJ) $(SYNC_OBJ))

# CXXFLAGS += -DST_DEBUG
CXXFLAGS += -g -O2 -I. -I../  -I../../../third_party/hiredis/include -std=c++0x
# CXXFLAGS += -g -I. -I../ -I../libtask -I/usr/local/include/hiredis -std=c++0x

LDFLAGS += ../st/libst.a ../../../third_party/hiredis/lib/libhiredis.a
# LDFLAGS += ../st/libst.a /usr/local/lib/libhiredis.a 

all: $(TARG) echo_server echo_client datagram_server

$(TARG): $(OBJS)
	$(AR) -rv $@ $^
	
$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

echo_client: test/echo_client.cpp
	g++ $(CXXFLAGS) -o$@ $^ libco2.a $(LDFLAGS) -ldl
	
echo_server: test/echo_server.cpp
	g++ $(CXXFLAGS) -o$@ $^ libco2.a $(LDFLAGS) -ldl

datagram_server: test/datagram_server.cpp
	g++ $(CXXFLAGS) -o$@ $^ libco2.a $(LDFLAGS) -ldl

clean:
	rm -rf $(OBJS) $(TARG) echo_server echo_client datagram_server 
