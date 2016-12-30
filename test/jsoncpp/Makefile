CXX=g++
CXXFLAGS=-I../../third_party/jsoncpp/include
LDFLAGS=-L ../../third_party/jsoncpp/lib/ -ljson_linux-gcc-4.8.5_libmt
FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
BINS=$(patsubst %.cpp, %, $(FILES))

all:$(BINS)

$(BINS):%:%.o
	$(CXX) -o $@ $< $(LDFLAGS)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(BINS)


include ../../Makefile.inc
