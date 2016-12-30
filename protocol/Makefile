NEED_PROTOBUF=1
TARGET=libposeidonproto.a
CXXFLAGS= -c -g -Wall -static -I. -I$(PROTOBUF_INC)
LDFLAG= -lprotobuf -lpthread
AUTO_GENERATE=auto_generate

BASE_DIR=./
PROTO_DIR=$(BASE_DIR)/proto
SRC_DIR=$(BASE_DIR)/src
OBJ_DIR=$(BASE_DIR)/src
LIB_DIR=$(BASE_DIR)/lib
PROTOC_BIN=protoc

PROTOFILE= $(shell find $(PROTO_DIR) -maxdepth 1 -name "*.proto" | xargs -L1 basename)
PROTOHEAD= $(patsubst %.proto, %.pb.h, $(PROTOFILE))
PROTOSRC= $(patsubst %.proto, %.pb.cc, $(PROTOFILE))
PROTOOBJ= $(patsubst %.proto, %.pb.o, $(PROTOFILE))

SUPPORT_SO=1

all: $(addprefix $(SRC_DIR)/,$(PROTOSRC)) $(addprefix $(LIB_DIR)/,$(TARGET)) $(AUTO_GENERATE)
$(LIB_DIR)/$(TARGET): $(addprefix $(OBJ_DIR)/,$(PROTOOBJ))
	mkdir -p $(LIB_DIR)	
	ar -rv $@ $^ 

$(OBJ_DIR)/%.pb.o:$(SRC_DIR)/%.pb.cc
	g++ -fPIC $(CXXFLAGS) -o $@  $<

$(SRC_DIR)/%.pb.cc:$(PROTO_DIR)/%.proto
	mkdir -p $(SRC_DIR) 
	$(PROTOC_BIN) -I$(dir $<) $< --cpp_out=$(SRC_DIR)

$(AUTO_GENERATE):
	@chmod +x auto_generate_include.sh && ./auto_generate_include.sh

clean :
	rm -f $(addprefix $(SRC_DIR)/,$(PROTOSRC) $(PROTOHEAD)) $(addprefix $(OBJ_DIR)/,$(PROTOOBJ)) $(addprefix $(LIB_DIR)/,$(TARGET))

include ../Makefile.inc
