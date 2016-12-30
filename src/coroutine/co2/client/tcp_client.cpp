/*
 * tcp_client.cpp
 * Created on: 2016-11-29
 */

#include "co2/client/tcp_client.h"

#include <functional>

#include "co2/co2.h"
#include "co2/stream/stream.h"
#include "co2/buffer/buffer.h"

namespace co2 {
namespace client {

using namespace co2;

co2::buffer::Pool *TCPClient::pool_ = 0;

TCPClient::TCPClient() : stream_(NULL)
{

}

TCPClient::~TCPClient(void)
{
    if (stream_) {
        stream_->Close();
        delete stream_;
    }
}

int TCPClient::Connect(const std::string &host, int port, int timeout)
{
    if (!pool_) {
        pool_ = new co2::buffer::Pool(16384, 10);
    }

    if (!stream_) {
        stream_ = new (std::nothrow) co2::stream::Stream(pool_);
        if (!stream_) {
            return kMemoryErr;
        }
    } else {
        Close();
    }
    return stream_->Connect(host, port, timeout);
}

int TCPClient::ConnectUnix(const std::string &path, int timeout)
{
    if (!pool_) {
        pool_ = new co2::buffer::Pool(16384, 10);
    }

    if (!stream_) {
        stream_ = new (std::nothrow) co2::stream::Stream(pool_);
        if (!stream_) {
            return kMemoryErr;
        }
    } else {
        Close();
    }
    return stream_->ConnectUnix(path, timeout);
}

int TCPClient::ReadBytes(int bytes, std::string &buf, int timeout)
{
    return stream_->ReadBytes(bytes, buf, timeout);
}

int TCPClient::ReadLine(std::string &buf, int timeout)
{
    return stream_->ReadLine(buf, timeout);
}

int TCPClient::WriteBytes(const char * buf, int size, int timeout)
{
    return stream_->WriteBytes(buf, size, timeout);
}

int TCPClient::WriteBytes(const std::string &buf, int timeout)
{
    return stream_->WriteBytes(buf.c_str(), buf.size(), timeout);
}

int TCPClient::AppendBytes(const char *buf, int size)
{
    return stream_->AppendBytes(buf, size);
}

int TCPClient::AppendBytes(const std::string &buf)
{
    return stream_->AppendBytes(buf.c_str(), buf.size());
}

int TCPClient::Flush(int timeout)
{
    return stream_->Flush(timeout);
}

int TCPClient::Close(void)
{
    if (stream_) {
        stream_->Close();
    }
    return 0;
}

}  // namespace client
}  // namespace co2


