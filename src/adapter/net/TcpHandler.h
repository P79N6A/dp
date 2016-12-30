#ifndef TCPHANDLER_H
#define TCPHANDLER_H
/**
    auth: xxxx
    date: 2016/05
**/
#include <boost/noncopyable.hpp>
#include <boost/pool/object_pool.hpp>

#include <muduo/base/Atomic.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoop.h>

#include "HttpRequest.h"


namespace poseidon
{

namespace adapter
{

class TcpHandler : boost::noncopyable
{
public:
    TcpHandler(OnBodyComplete &bodycompleteCb, OnRequestComplete &requestcompleteCb);
    virtual ~TcpHandler();

    void OnConnection(const muduo::net::TcpConnectionPtr&);
    void OnMessage(const muduo::net::TcpConnectionPtr&,
                              muduo::net::Buffer*,
                              muduo::Timestamp);
private:
    muduo::AtomicInt32 connections_;
};

}
}

#endif // HTTPHANDLER_H
