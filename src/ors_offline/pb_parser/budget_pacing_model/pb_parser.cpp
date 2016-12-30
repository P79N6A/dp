#include "util/log.h"
#include "protocol/src/poseidon_proto.h"
#include "util/proto_helper.h"


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Invalid paramters!\n");
		fprintf(stdout, "Usage:%s [pb file]", argv[0]);
	}
	poseidon::ors::BudgetPacingModel message;
	if (poseidon::util::ParseProtoFromBinaryFormatFile(argv[1], &message))
	{
		message.PrintDebugString();
	}

	return 0;
}
