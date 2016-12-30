/**
**/

#ifndef _UTIL_PROTO_HELPER_H_
#define _UTIL_PROTO_HELPER_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "third_party/google/protobuf/message.h"

namespace poseidon
{
namespace util
{
typedef ::google::protobuf::Message ProtoMessage;

bool ParseProtoFromTextFormatFile(const char* file, ProtoMessage* message);
bool ParseProtoFromBinaryFormatFile(const char* file, ProtoMessage* message);

} // namespace ors
} // namespace poseidon

#endif // _UTIL_PROTO_HELPER_H_

