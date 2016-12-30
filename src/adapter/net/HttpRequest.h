#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
/**
    auth: xxxx
    date: 2016/05
**/
#include <map>
#include <vector>
#include <string>

#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <muduo/net/InetAddress.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>

#include "common/common.h"
#include "HttpParser.h"

namespace poseidon
{

namespace adapter
{

class HttpRequest;


typedef boost::weak_ptr<muduo::net::TcpConnection> TcpConnectionWptr;
typedef boost::function<void(const char*, size_t, const HttpRequest*)> OnBodyComplete;
typedef boost::function<void(const HttpRequest*)> OnRequestComplete;

/*
此对象的生命周期与一个连接的生命周期相同
*/
class HttpRequest : boost::noncopyable
{
public:
    explicit HttpRequest(const muduo::net::TcpConnectionPtr &);
    virtual ~HttpRequest();
    size_t ParseHttp(const muduo::net::InetAddress &remote, const char *, size_t);
    const muduo::net::Buffer& BodyBuff() const { return body_; }
    //const std::multimap<std::string, std::string>& Headers() const { return headers_; }
    const std::map<std::string, std::string>& Headers() const { return headers_; }

    TcpConnectionWptr GetWConnection() const { return wconn_; }
    const std::string& HttpRequestUrl() const { return http_request_url_; }
    static void SetHttpRequestTimeOut(double timeout) { http_req_timeout_ = timeout; }

    static void SetBodyCompleteCb(OnBodyComplete cb) { bc_ = cb; }
    static void SetRequestCompleteCb(OnRequestComplete cb) { rc_ = cb; }
private:
    static OnRequestComplete rc_;
    static OnBodyComplete  bc_;
    static double http_req_timeout_;

    TcpConnectionWptr wconn_;//用来在接收请求头的时候做超时判断。不能保存TcpConnectionPtr，因为会导致连接无法释放
    std::string http_header_current_; //用来保存解析到的head名字
    //muduo::net::TcpConnectionPtr conn_;
    muduo::net::TimerId request_complete_;//
    struct http_parser parser_;
    struct http_parser_settings setting_;//对大并发情况，为节省内存，可把此结构移至httpHandler中
    std::vector<std::string> keys_;
    //std::multimap<std::string, std::string> headers_;
    std::map<std::string, std::string> headers_;
    muduo::net::Buffer body_;//post方法对应的body数据 TODO:直接使用connection的buff？
    std::string http_request_url_;//每个http请求时的url均保存在此

    void InitParser();
    //超时检查必须是静态方法。因为调用此函数时，对应的HttpRequest可能已经释放了（tcp连接已经关闭）
    static void OnRequestTimeOut(TcpConnectionWptr wptr, HttpRequest *instance);//当请求端的http请求不完全，且超时时的处理

///////////////httpparser callback/////////////////////////////////
    static int OnHttpMessageBegin(http_parser *parser);
    static int OnHttpUrl(http_parser *parser, const char *at, size_t length);
    static int OnHttpHeaderField(http_parser *parser, const char *at, size_t length);
    static int OnHttpHeaderValue(http_parser *parser, const char *at, size_t length);
    static int OnHttpHeadersComplete(http_parser *parser);
    static int OnHttpBody(http_parser *parser, const char *at, size_t length);
    static int OnHttpMessageComplete(http_parser *parser);
///////////////httpparser end//////////////////////////////////////
};

}

}

#endif // HTTPREQUEST_H
