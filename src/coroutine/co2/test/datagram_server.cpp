/*
 * datagram_server.cpp
 * Created on: 2016-12-12
 */

#include "co2/co2.h"
#include "co2/server/udp_server.h"

using namespace co2;

class EchoServer : public co2::server::UDPServer {
public:
    EchoServer();
    void OnTimer(void *arg);
    virtual int OnDatagram(const std::string &host, int port,
        const char *buf, int size);
};

EchoServer::EchoServer() : co2::server::UDPServer()
{

}

void EchoServer::OnTimer(void *arg)
{
    fprintf(stderr, "ontimer ...\n");
}

int EchoServer::OnDatagram(const std::string &host, int port,
    const char *buf, int size)
{
    /* echo back */
    WriteBytes(host, port, buf, size);
    return 0;
}

int main(int argc, char *argv[])
{
    co2::Init();
    EchoServer server;
    server.Bind("127.0.0.1", 10000);
    server.ServeForever();
    exit(0);
}
