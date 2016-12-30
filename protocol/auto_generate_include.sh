#!/bin/bash

PROTO_DIR=./proto
PROTO_FILE=`find ${PROTO_DIR} -maxdepth 1 -name "*.proto" | xargs -L1 basename | cut -d \. -f 1 | sort`

TARGET_FILE="./src/poseidon_proto.h"
INCLUDE_PRE="protocol/src"

function SH_declare()
{
    echo "// Generated by the auto_generate_include.sh.  DO NOT EDIT!"
}

(
SH_declare;
echo "";
echo "#ifndef POSEIDON_PROTO_H_";
echo "#define POSEIDON_PROTO_H_";
echo "";

for PROTO in ${PROTO_FILE}
do
    echo "#include \"${INCLUDE_PRE}/${PROTO}.pb.h\""
done

echo "";
echo "#endif";
) > ${TARGET_FILE}