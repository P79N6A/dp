/*
 * tcp_client.h
 * Created on: 2016-11-29
 */

#ifndef CO2_CLIENT_TCP_CLIENT_H_
#define CO2_CLIENT_TCP_CLIENT_H_

#include <string>

#include "co2/stream/stream.h"
#include "co2/buffer/buffer.h"

namespace co2 {
namespace client {

class TCPClient {
public:
    TCPClient();
    virtual ~TCPClient();

    int Connect(const std::string &host, int port, int timeout = -1);
    int ConnectUnix(const std::string &path, int timeout = -1);
    int Bind(const std::string &addr, int port);
    int ReadBytes(int bytes, std::string &buf, int timeout = -1);
    int ReadLine(std::string &buf, int timeout = -1);
    int WriteBytes(const char * buf, int size, int timeout = -1);
    int WriteBytes(const std::string &buf, int timeout = -1);
    int AppendBytes(const char *buf, int size);
    int AppendBytes(const std::string &buf);
    int Flush(int timeout = -1);
    int Close();

public:
    static co2::buffer::Pool *pool_;

protected:
    TCPClient &operator = (const TCPClient &) = delete;
    TCPClient(const TCPClient &client) = delete;

protected:
    co2::stream::Stream *stream_;
};



}}

#endif /* CO2_CLIENT_TCP_CLIENT_H_ */
