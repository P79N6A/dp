PROTO_COMPILER=`which protoc`
SRC_DIR = .
PROTO_SRC_DIR = .
DEST_DIR = .

all: tanx-bidding_pb2.py

tanx-bidding_pb2.py: tanx-bidding.proto
	$(PROTO_COMPILER) -I=$(SRC_DIR) --python_out=$(DEST_DIR) tanx-bidding.proto
