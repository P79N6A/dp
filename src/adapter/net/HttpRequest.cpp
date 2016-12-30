#include "HttpRequest.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>

#include <muduo/base/Logging.h>

#include "conf/Configer.h"
#include "monitor_api.h"//MON_ADD

#include "common/common.h"

/**
    auth: xxxx
    date: 2016/05
**/

using namespace poseidon;
using namespace poseidon::adapter;

extern Configer * volatile configer;

double HttpRequest::http_req_timeout_;
OnRequestComplete HttpRequest::rc_;
OnBodyComplete  HttpRequest::bc_;

HttpRequest::HttpRequest(const muduo::net::TcpConnectionPtr &conn) : wconn_(conn)
{
    http_parser_settings_init(&setting_);
    setting_.on_message_begin = &HttpRequest::OnHttpMessageBegin;
    setting_.on_url = &HttpRequest::OnHttpUrl;
    setting_.on_header_field = &HttpRequest::OnHttpHeaderField;
    setting_.on_header_value = &HttpRequest::OnHttpHeaderValue;
    setting_.on_headers_complete = &HttpRequest::OnHttpHeadersComplete;
    setting_.on_message_complete = &HttpRequest::OnHttpMessageComplete;
    setting_.on_body = &HttpRequest::OnHttpBody;

    InitParser();
    parser_.data = this;
}

HttpRequest::~HttpRequest()
{
    //dtor
}


void HttpRequest::InitParser()
{
    http_parser_init(&parser_, HTTP_REQUEST);//parser_.data is saved!
    if (headers_.size() > 30) { //防止head太多
        headers_.clear();
    } else { //非必要，不清空整个head，只复位value
        auto iter = headers_.begin();
        for (; iter != headers_.end(); ++iter) {
            iter->second.clear();
        }
    }
    body_.retrieveAll();
    keys_.clear();
    http_request_url_.clear();
    http_header_current_.clear();
}


size_t HttpRequest::ParseHttp(const muduo::net::InetAddress &remote, const char *buf, size_t len)
{
    size_t preceed = http_parser_execute(&parser_, &setting_, buf, len);
    if (preceed == 0 || HTTP_PARSER_ERRNO(&parser_) != HPE_OK) {//invalid request
        LOG_ERROR << "Parse Http Request failed:" << http_errno_description(HTTP_PARSER_ERRNO(&parser_)) <<
                  ". preceed size:" << preceed << ". Peer IP:" << remote.toIp();;
        return 0;//will be closed!
    }
    return preceed;
}

//当客户端故意不发送完整的http请求头时，经过超时时长，会调用此函数
//此函数会判断并决定是否关闭当前连接！
void HttpRequest::OnRequestTimeOut(TcpConnectionWptr wptr, HttpRequest *instance)
{
    boost::shared_ptr<muduo::net::TcpConnection> ptr = wptr.lock();
    //ptr生命周期管理instance
    if(ptr) {
        MON_ADD(ATTR_ADAPTER_HTTP_REQUEST_TIMEOUT, 1);
        LOG_WARN << "Connection:" << ptr->name() << " timeout! close it.";
        ptr->forceClose();//处理请求头超时，那么关闭当前连接！
    }
}


/////////////////httpparse callback/////////////////////////////

int HttpRequest::OnHttpMessageBegin(http_parser *parser)
{
    //LOG_TRACE << "OnHttpMessageBegin";
    HttpRequest *instance = static_cast<HttpRequest*>(parser->data);

    if(instance->http_req_timeout_ > 0.01) {
        //注意，eventloop执行OnRequestTimeOut时，由于延迟原因，当前httprequest对象可能已经销毁！
        //故在此用weak_ptr来观察此对象是否存活
        instance->request_complete_ = instance->wconn_.lock()->getLoop()->runAfter(
                                          instance->http_req_timeout_,
                                          //必须静态方法。因为执行此函数时，HttpRequest可能已经被释放了
                                          //如果是类成员方法的话，调用已释放的成员函数会内存访问违规
                                          bind(&HttpRequest::OnRequestTimeOut,
                                               instance->wconn_, instance));
    }
    return 0;
}
int HttpRequest::OnHttpUrl(http_parser *parser, const char *at, size_t length)
{
    HttpRequest *instance = static_cast<HttpRequest*>(parser->data);
    //size_t type is equal zero or great then zero
    //typdef unsigned int size_t
    if(length <= 0 || length > HTTP_MAX_URL_LENGTH) {
        LOG_ERROR << "Http url length:" << length;
        return -1;
    }

    //std::string url(at, length);
    //eg:http://10.15.19.165:8080/adx/youtu/v1  --->url = /adx/youtu/v1
    //eg:http://10.15.19.165:8080  --->url = /
    //instance->headers_.insert(make_pair("HTTPURL", url));
    instance->http_request_url_.assign(at, length);//考虑httpurl后面一定要使用，故不加入map中，直接用变量保存

    return 0;
}

int HttpRequest::OnHttpHeaderField(http_parser *parser, const char *at, size_t length)
{
    HttpRequest *instance = static_cast<HttpRequest*>(parser->data);
    if(length <= 0 || length > HTTP_MAX_HEAD_NAME_LENGTH) {
        LOG_ERROR << "Http head_field length:" << length;
        return -1;
    }
    //instance->keys_.push_back(std::string());
    //std::string &back = instance->keys_.back();
    //back.assign(at, length);
    //boost::to_lower(back);
    instance->http_header_current_.assign(at, length);
    boost::to_lower(instance->http_header_current_);
    return 0;
}

int HttpRequest::OnHttpHeaderValue(http_parser *parser, const char *at, size_t length)
{
    HttpRequest *instance = static_cast<HttpRequest*>(parser->data);
    if(length <= 0 || length > HTTP_MAX_HEAD_VALUE_LENGTH) {
        LOG_ERROR << "Http head_value length:" << length;
        return -1;
    }
    /*
    if(instance->keys_.size() > 1) {
        LOG_WARN << "HTTP: > 1 keys!";
    }
    if(instance->keys_.size() == 0) {
        LOG_WARN << "HTTP: == 0 keys!";
        return 0;
    }
    std::string &value = instance->headers_[instance->keys_.back()];
    value.assign(at, length);
    boost::to_lower(value);
    instance->keys_.pop_back();
    */
    if (instance->http_header_current_.length() > 0) {
        std::string &value = instance->headers_[instance->http_header_current_];
        value.assign(at, length);
        boost::to_lower(value);
        instance->http_header_current_.clear();
    }
    return 0;
}

int HttpRequest::OnHttpHeadersComplete(http_parser *parser)
{
    //HttpRequest *instance = static_cast<HttpRequest*>(parser->data);
    //LOG_TRACE << "OnHttpHeadersComplete heads:" << instance->headers_.size();
    //printf("head size:%d\n", instance->headers_.size());
    return 0;
}

int HttpRequest::OnHttpBody(http_parser *parser, const char *at, size_t length)
{
    //检查单次大小是否过大
    if(length <= 0 || length > HTTP_MAX_BODY_LENGTH) {
        LOG_ERROR << "Http once request body size are too big!";
        return -1;
    }
    HttpRequest *instance = static_cast<HttpRequest*>(parser->data);
    instance->body_.append(at, length);
    //检查若body大小是否超额
    if(instance->body_.readableBytes() > HTTP_MAX_BODY_LENGTH) {
        LOG_ERROR << "Http Body size exceed limited!";
        return -1;
    }
    //检查body是否已经全部收取。1为完成
    int complete = http_body_is_final(parser);
    if(complete && instance->bc_) {
        instance->bc_(instance->body_.peek(), instance->body_.readableBytes(), instance);
    }
    return 0;
}

int HttpRequest::OnHttpMessageComplete(http_parser *parser)
{
    //LOG_TRACE << "OnHttpMessageComplete";
    HttpRequest *instance = static_cast<HttpRequest*>(parser->data);
    instance->wconn_.lock()->getLoop()->cancel(instance->request_complete_);
    if(instance->rc_) {
        instance->rc_(instance);
    }
    //printf("OnHttpMessageComplete\n");
    //重置parser，准备下一次请求接收
    instance->InitParser();
    return 0;
}

//////////////////////////httpparse end///////////////////////////////
