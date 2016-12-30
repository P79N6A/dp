/*
 * test_server.cpp
 * Created on: 2016-11-21
 */

#include "co2/co2.h"
#include "co2/stream/stream.h"
#include "co2/server/tcp_server.h"
#include "co2/timer/timer.h"

using namespace co2;

class EchoServer : public co2::server::TCPServer {
public:
    EchoServer();
    void OnTimer(void *arg);
    virtual int OnAccept(co2::stream::Stream &stream);
};

EchoServer::EchoServer() : co2::server::TCPServer(256 * 1024)
{

}

void EchoServer::OnTimer(void *arg)
{
    fprintf(stderr, "ontimer ...\n");
}

int EchoServer::OnAccept(co2::stream::Stream &stream)
{
    fprintf(stderr, "OnAccept\n");
    std::string buf;
    while (true) {
        int res = stream.ReadLine(buf);

        fprintf(stderr, "res = %d, size = %d, buf = %s\n", res, buf.size(), buf.c_str());
        if (res != kOK) {
            if (res == kClosedErr) {
                fprintf(stderr, "client closed\n");
                stream.Close();
            } else if (res == kTimeoutErr) {
                fprintf(stderr, "client timeout\n");
                stream.Close();
            } else if (res == kReadErr) {
                fprintf(stderr, "client error\n");
                stream.Close();
            }
            break;
        }
        if (buf == "quit\n" || buf == "quit\r\n") {
            stream.Close();
            Stop();
            break;
        } else {
            stream.WriteBytes(buf);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    co2::Init();

    EchoServer server;
    int res = server.Bind("127.0.0.1", 10000);
    if (res != 0) {
        fprintf(stderr, "bind 10000 failed\n");
        return -1;
    } else {
        /* start listening but not start the ioloop yet. */
        server.Start();
    }

    EchoServer server2;
    co2::timer::Timer * timer = co2::timer::Timer::Create(
        &EchoServer::OnTimer, server2, 1000, NULL);

    /* start timer before calling ServeForever. */
    // timer->Start();

    res = server2.BindUnix("/tmp/echo");
    if (res != 0) {
        fprintf(stderr, "bind unix failed\n");
        return -1;
    } else {
        /* use serve forever to start the coroutine ioloop */
        server2.ServeForever();
    }

    /* destroy timer */
    delete timer;
    fprintf(stderr, "exiting ...\n");
    exit(0);
}
