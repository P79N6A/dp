NEED_PROTO=1
NEED_BOOST=1
NEED_LOG=1

FILES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(FILES))
TARG1=libapi.a
DEPLIB+=../context/libscoringcontext.a
DEPLIB+=../scorer/libscoringscorer.a
ADDLIB+=addlib ../context/libscoringcontext.a\n
ADDLIB+=addlib ../scorer/libscoringscorer.a\n
ADDLIB+=addlib $(TARG1)\n
TARG=libscoringapi.a
all:$(TARG)

$(OBJS):%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(TARG):$(OBJS) $(DEPLIB)
	ar -rv $(TARG1) $^
	echo -e 'create $@\n$(ADDLIB)save\nend' | ar -M

clean:
	rm  -f $(OBJS) $(TARG) $(TARG1)

include ../../../Makefile.inc

