NEED_PROTO=1
NEED_LOG=1
NEED_BOOST=1
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
TARG=liborsranking.a
all:$(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS)
	ar -rv $@ $^

clean:
	rm  -f $(OBJS) $(TARG)

include ../../../Makefile.inc

