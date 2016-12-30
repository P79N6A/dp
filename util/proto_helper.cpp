/**
**/

//include STD C/C++ head files
#include <fstream>

//include third_party_lib head files
#include "util/proto_helper.h"
#include "third_party/google/protobuf/text_format.h"
#include "third_party/google/protobuf/io/coded_stream.h"
#include "third_party/google/protobuf/io/zero_copy_stream_impl.h"
#include "util/log.h"

namespace poseidon
{
namespace util
{

bool ParseProtoFromTextFormatFile(const char* file, ProtoMessage* message)
{
    std::ifstream ifs(file, std::ios::in);
    if (!ifs.is_open())
    {
        LOG_ERROR("open file=%s Failed!", file);
        return false;
    }

    ::google::protobuf::io::ZeroCopyInputStream *file_io = new ::google::protobuf::io::IstreamInputStream(&ifs);    
    if (!::google::protobuf::TextFormat::Parse(file_io, message))
    {
        LOG_ERROR("Parse file=%s Failed!",file);
        ifs.close();
        return false;
    }
    delete file_io;

    ifs.close();
    return true;
}


bool ParseProtoFromBinaryFormatFile(const char* file, ProtoMessage* message)
{
    std::ifstream ifs(file, std::ios::in|std::ios::binary);    
    if (!ifs.is_open()) 
    {   
        LOG_ERROR("open file=%s Failed!", file);
        return false;    
    }    
    ::google::protobuf::io::IstreamInputStream file_input(&ifs);    
    ::google::protobuf::io::CodedInputStream coded_input(&file_input); 
    coded_input.SetTotalBytesLimit(1024*1024*1024, 640*1024*1024);    
    if (!message->ParseFromCodedStream(&coded_input) || !coded_input.ConsumedEntireMessage())
    {        
        LOG_ERROR("Parse file=%s Failed!", file);                
        ifs.close();        
        return false;    
    }    
    ifs.close();    
    
    return true;
}


} // namespace util
} // namespace poseidon

