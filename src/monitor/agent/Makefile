NEED_BOOST=1
NEED_LOG=1
NEED_PROTO=1
NEED_SSL=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
TARG=monitor_agent


all:$(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	rm -f $(ROOT_DIR)/envs/poseidon/bin/$(TARG)
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
	tar -cvzf $(TARG).tar.gz $(TARG)
	md5sum $(TARG)

clean:
	rm  -f $(OBJS) $(TARG)

install:
	cp -r $(TARG) $(ROOT_DIR)/envs/poseidon/bin/
include ../../../Makefile.inc

