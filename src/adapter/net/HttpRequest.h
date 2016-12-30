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
�˶��������������һ�����ӵ�����������ͬ
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

    TcpConnectionWptr wconn_;//�����ڽ�������ͷ��ʱ������ʱ�жϡ����ܱ���TcpConnectionPtr����Ϊ�ᵼ�������޷��ͷ�
    std::string http_header_current_; //���������������head����
    //muduo::net::TcpConnectionPtr conn_;
    muduo::net::TimerId request_complete_;//
    struct http_parser parser_;
    struct http_parser_settings setting_;//�Դ󲢷������Ϊ��ʡ�ڴ棬�ɰѴ˽ṹ����httpHandler��
    std::vector<std::string> keys_;
    //std::multimap<std::string, std::string> headers_;
    std::map<std::string, std::string> headers_;
    muduo::net::Buffer body_;//post������Ӧ��body���� TODO:ֱ��ʹ��connection��buff��
    std::string http_request_url_;//ÿ��http����ʱ��url�������ڴ�

    void InitParser();
    //��ʱ�������Ǿ�̬��������Ϊ���ô˺���ʱ����Ӧ��HttpRequest�����Ѿ��ͷ��ˣ�tcp�����Ѿ��رգ�
    static void OnRequestTimeOut(TcpConnectionWptr wptr, HttpRequest *instance);//������˵�http������ȫ���ҳ�ʱʱ�Ĵ���

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
