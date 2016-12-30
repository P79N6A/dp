#include "qp_worker.h"
#include "attr.h"
#include "qp_device_price_map.h"
#include "dmp_util.h"
#include "ip_search.h"

DEFINE_int32(qp_context_timeout, 10, "QP处理超时.单位:微秒");
DEFINE_string(tag_redis_host, "127.0.0.1", "redis host");
DEFINE_int32(tag_redis_port, 6381, "redis port");
DEFINE_bool(tag_redis_cluster, false, "是否使用redis集群");
DEFINE_int32(test_md5_rate, 0, "设置imei转换为md5的比例，测试md5(imei)的映射功能");

namespace poseidon {
namespace qp {

void udp_recv_cb(evutil_socket_t fd, short event, void *arg) {
    if (!(event & EV_READ))
        return;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buffer[UDP_SERV_RECV_BUFFER_SIZE] = { 0 };
    int recv_len = recvfrom(fd, buffer, sizeof(buffer), 0,
            (struct sockaddr *) &addr, &addr_len);
    if (recv_len < 0) {
        return;
    }
    MON_ADD(ATTR_QP_REQ, 1);

    QpWorker * worker = (QpWorker *) arg;
    worker->process(buffer, recv_len, addr);
}

void context_timeout_cb(evutil_socket_t fd, short event, void *arg) {
    util::Closure *timeout_process = (util::Closure *) arg;
    timeout_process->Run();
}

void QpWorker::start_up(int sock_fd) {
    _sock_fd = sock_fd;
    _base = event_base_new();
    if (_base == NULL) {
        LOG_ERROR("worker start up error,event can not new");
        exit(-1);
    }

    struct event *udp_recv_event = new event;
    event_assign(udp_recv_event, _base, _sock_fd, EV_PERSIST | EV_READ,
            udp_recv_cb, (void *) this);
    event_add(udp_recv_event, NULL);

    _redis_client.init(_base, FLAGS_tag_redis_host, FLAGS_tag_redis_port,
            FLAGS_tag_redis_cluster);

    event_base_dispatch(_base);
}

void QpWorker::process(char *recv_buf, int recv_len,
        const struct sockaddr_in &addr) {
    boost::shared_ptr<QpContext> context(new QpContext);
    context->serial_num = _serial_num++;
    if (!context->qp_request.ParseFromArray(recv_buf, recv_len)) {

        MON_ADD(ATTR_QP_PARSE_ERR, 1);
        LOG_ERROR("qp_request parse error,recv_len=%d", recv_len);
        return;
    }
    context->interval("parse_pb_time");
    set_timeout(context);
    context->req_size = recv_len;
    memcpy(&(context->peer_addr), &addr, sizeof(struct sockaddr_in));

    if (context->qp_request.has_trace_id()) {
        context->id = context->qp_request.trace_id();
    } else {
        context->id = util::Func::to_str(_serial_num);
    }
    if (!add_context_map(context)) {
        LOG_ERROR("multi context id[%s]", context->id.c_str());
        return;
    }
    LOG_DEBUG("\r\nresquest begin------\r\n %s\r\nresquest end------",
            context->qp_request.DebugString().c_str());
    context->interval("init_time");

    if (!context->qp_request.has_device()
            || !context->qp_request.device().has_id()) {
        MON_ADD(ATTR_QP_NO_DEV_ID, 1);
    } else {
        context->imei = boost::to_lower_copy(context->qp_request.device().id());

        if (context->serial_num % 100 < FLAGS_test_md5_rate) {
            if (context->imei.length() > 0) {
                string md5_str;
                context->imei = util::Func::md5sum(context->imei.c_str(),
                        context->imei.length(), md5_str);
                context->imei = md5_str;
            }
        }

        if (context->imei.length() == 14 || context->imei.length() == 15) {
            context->need_redis_tags = true;
            get_user_tag(context);
        } else if (context->imei.length() == 32) {
            MON_ADD(ATTR_MD5_IMEI, 1);
            context->imei_md5 = context->imei;
            context->need_redis_tags = true;
            get_user_id(context);
        } else {
            MON_ADD(ATTR_ILLEGAL_IMEI, 1);
        }
    }

    get_local_tag(context);
    context->interval("local_tag_time");
    get_price_tag(context);
    context->interval("price_tag_time");
    get_ip_tag(context);
    context->interval("ip_tag_time");
    context->got_local_tags = true;
    done(context);
}
bool QpWorker::add_context_map(boost::shared_ptr<QpContext> context) {
    ContextMapIter iter = _context_map.find(context->id);
    if (iter != _context_map.end()) {
        return false;
    }
    _context_map[context->id] = context;

    return true;
}

void QpWorker::set_timeout(boost::shared_ptr<QpContext> context) {
    util::Closure *timeout_process = util::NewCallback(this,
            &QpWorker::timeout_process, context);
    context->timeout_event = new util::TimeoutEvent(_base, 0,
            FLAGS_qp_context_timeout * 1000, context_timeout_cb,
            (void*) timeout_process);
    context->timeout_event->active();
}

void QpWorker::timeout_process(boost::shared_ptr<QpContext> context) {
    context->interval("timeout_time");
    context->is_timeout = true;
    done(context);
}

void QpWorker::done(boost::shared_ptr<QpContext> context) {
    if (context->is_timeout) {

    } else {
        if (context->got_local_tags) {
            if (context->need_redis_tags) {
                if (context->got_redis_tags) {

                } else {
                    return;
                }
            }
        } else {
            return;
        }
    }
    context->interval("done");
    QPRequest & qpreq = context->qp_request;
    QPResponse & qprsp = context->qp_response;
    ContextMapIter iter = _context_map.find(context->id);
    if (iter == _context_map.end()) {
        return;
    }
    _context_map.erase(iter);
    if (context->is_timeout) {
        LOG_ERROR("%s is timeout", context->id.c_str());
    }
    while (context->code == common::ERROR_NONE) {
        if (context->has_tags) {
            DmpInfo * dmpinfo = qprsp.mutable_dmp_info();
            boost::unordered_map<int, vector<string> >::iterator iter;
            for (iter = context->qp_tag_infos.begin();
                iter != context->qp_tag_infos.end(); iter++) {
                
                common::Targetting * tag = dmpinfo->add_user_targets();
                tag->set_type(iter->first);
                for (int i = 0; i < iter->second.size(); i++)
                    tag->add_value(iter->second[i]);

            }
            //dmpinfo->mutable_ors_targets()->CopyFrom(dmpinfo->user_targets());
        } else {
            context->code = common::ERROR_NO_RESULT;
            break;
        }
        break;
    }
    qprsp.set_error_code(context->code);
    if (qpreq.has_session_id())
        qprsp.set_session_id(qpreq.session_id());
    if (qpreq.has_trace_id())
        qprsp.set_trace_id(qpreq.trace_id());
    if (context->imei.length() > 0)
        qprsp.set_imei(context->imei);
    char hostname[256] = { 0 };
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        qprsp.set_hostname(hostname);
    }
    do {
        char buff[UDP_SERV_SEND_BUFFER_SIZE] = { 0 };
        context->rsp_size = qprsp.ByteSize();
        if (context->rsp_size > sizeof(buff)) {
            LOG_ERROR("%s serialize error", context->id.c_str());
            break;
        }
        LOG_DEBUG("\r\nresponse begin------\r\n %s\r\nresponse end------",
                qprsp.DebugString().c_str());
        qprsp.SerializeToArray(buff, sizeof(buff));
        context->interval("pack_time");
        context->send_size = sendto(_sock_fd, buff, context->rsp_size, 0,
                (struct sockaddr*) &context->peer_addr,
                sizeof(struct sockaddr_in));
        if (context->send_size != context->rsp_size) {
            LOG_ERROR("%s udp send error", context->id.c_str());
            break;
        } else {
            MON_ADD(ATTR_QP_RSP, 1);
        }
    } while (0);
    context->stop();
}

void QpWorker::get_local_tag(boost::shared_ptr<QpContext> context) {
    QPRequest & qpreq = context->qp_request;
    const rtb::Device & device = qpreq.device();
    if (device.has_os()) {
        context->add_tag_value(QP_TAGID_OS,
                DmpUtil::os_to_id(
                        util::UtilStr::inst(device.os()).trim().to_upper().str()));
    }

    context->add_tag_value(QP_TAGID_NETWORK,
            DmpUtil::connection_type_to_id(device.connection_type()));

    if (qpreq.has_site()) {
        const rtb::Site & site = qpreq.site();
        size_t categories_size = site.page_categories_size();
        for (size_t i = 0; i < categories_size; i++) {
            int categ = site.page_categories(i);
            context->add_tag_value(QP_TAGID_CONTENT_CATEGORIES, categ);
            context->add_tag_value(QP_TAGID_CATEGORY, categ);
        }
    } else if (qpreq.has_app()) {
        const rtb::App & app = qpreq.app();
        size_t categories_size = app.page_categories_size();
        for (size_t i = 0; i < categories_size; i++) {
            int categ = app.page_categories(i);
            context->add_tag_value(QP_TAGID_APP_CATEGORIES, categ);
        }
    }
}

void QpWorker::get_ip_tag(boost::shared_ptr<QpContext> context) {
    QPRequest & qpreq = context->qp_request;
    if (qpreq.has_device() && qpreq.device().has_ip()) {
        uint32_t city_code = 0;
        uint32_t carrier_code = 0;
        int ret = IpSearch::get_mutable_instance().search(qpreq.device().ip(),
                city_code, carrier_code);
        if (ret == 0) {
            context->add_tag_value(QP_TAGID_LOCATION, city_code);
            LOG_DEBUG("%s in %d", qpreq.device().ip().c_str(), city_code);
        } else if (ret == -4 || ret == -5) {
            LOG_DEBUG("%s can not found", qpreq.device().ip().c_str());
            MON_ADD(ATTR_IP_TAG_NO_RESULT, 1);
        } else {
            LOG_ERROR("IP search error,ret=%d", ret);
        }
    }
}

void QpWorker::get_price_tag(boost::shared_ptr<QpContext> context) {
    QPRequest & qpreq = context->qp_request;
    string brand = qpreq.device().brand();
    string model = qpreq.device().model();
    string price_key = util::UtilStr::inst(brand).to_lower().str() + "`"
            + util::UtilStr::inst(model).to_lower().str();
    LOG_DEBUG("%s get_price_tag key : %s", context->id.c_str(),
            price_key.c_str());
    DeviceData price_data;
    if (DevicePriceMap::get_mutable_instance().get_price_tag(price_key,
            price_data)) {
        MON_ADD(ATTR_DEV_TAG_HAVE_DATA, 1);
        if (price_data->has_brand()) {
            context->add_tag_value(QP_TAGID_BRAND, price_data->brand());
        }
        if (price_data->has_model()) {
            context->add_tag_value(QP_TAGID_MODEL, price_data->model());
        }
        if (price_data->has_price()) {
            context->add_tag_value(QP_TAGID_DEVICE_PRICE, price_data->price());
        }
    } else {
        MON_ADD(ATTR_DEV_TAG_NO_KEY, 1);
    }
}

}
}
