
/***********************************************************************

Copyright 2017 quantOS-org

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at:

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

***********************************************************************/
#include <chrono>
#include "base/Logger.h"
#include "ServerImpl.h"
#include "Event.h"
#include "unistd.h"
#include "zmq.hpp"
#include "MsgZmqCtx.h"
#include "zhelpers.hpp"
#include "msgbus/MsgQueue.h"
#include "protocol/cpp/jzs.h"

#include "ServerImpl.h"

using namespace jzs::msg;

using namespace std;
using namespace std::chrono;
using namespace jzs;
using namespace jzs::msgbus;

Server* Server::create(const char* local_addr, const char* id, ServerCallback* cb)
{
    return new ServerImpl(local_addr, id, cb);
}

ServerImpl::ServerImpl(const char* local_addr, const char* id, ServerCallback* cb):
    m_id(id),
    m_local_addr(local_addr),
    m_cb(cb)
{
    m_backend_addr = string("inproc://server_proxy_") + id;

    m_frontend_sock = new zmq::socket_t(*msgbus::get_ctx(), ZMQ_ROUTER);
    m_backend_sock = new zmq::socket_t(*msgbus::get_ctx(), ZMQ_DEALER);

    m_frontend_sock->bind(m_local_addr.c_str());
    m_backend_sock->bind(m_backend_addr.c_str());

    m_queue_addr = string("inproc://server_queue_") + id;
    m_queue_sock = new zmq::socket_t(*msgbus::get_ctx(), ZMQ_PUSH);
    m_queue_sock->bind(m_queue_addr.c_str());

    m_worker_sock = new zmq::socket_t(*msgbus::get_ctx(), ZMQ_DEALER);
    m_worker_sock->connect(m_backend_addr.c_str());
}

ServerImpl::~ServerImpl()
{
    if (m_worker_thread)
        delete m_worker_thread;
    if (m_proxy_thread)
        delete m_proxy_thread;

    delete m_frontend_sock;
    delete m_backend_sock;
    delete m_queue_sock;
}

void ServerImpl::start()
{
    m_worker_thread = new std::thread(std::bind(&ServerImpl::worker_run, this));
    m_proxy_thread = new std::thread(std::bind(&ServerImpl::proxy_run, this));
}

// must to call to enable zmq::proxy
void ServerImpl::proxy_run()
{
    while(true) {
        try {
            zmq::proxy(*m_frontend_sock, *m_backend_sock, nullptr);
        }catch(std::exception& e) {
            LOG(ERROR)<<"zmq::proxy throw expection " << e.what();
            usleep(200);
        }
    }
}

void ServerImpl::stop()
{
    LOG(FATAL) << "TO BE IMPLEMENTED";
}

void ServerImpl::process_request(zmq::socket_t* sock)
{
    zmq::message_t identity;
    zmq::message_t msg;
    int more;
    size_t more_size = sizeof(more);


    if (!sock->recv(&identity, ZMQ_DONTWAIT))
        return;

    sock->getsockopt(ZMQ_RCVMORE, &more, &more_size);
    if (!more) {
        LOG(ERROR) << "Receive uncomplete packet";
        return;
    }

    // FIXME: 这两个消息应该同时到达，如果不是，则会出错！
    if (!sock->recv(&msg, ZMQ_DONTWAIT)) {
        LOG(ERROR) << "Packet has no body";
        return;
    }

    Msg jmsg;
    if (!jmsg.ParseFromArray(msg.data(), msg.size()))
        return;

    if (jmsg.head().tid() == MsgType::MSG_LOW_HEARTBEAT_REQ) {
        //LOG(WARNING) << "Recv MSG_LOW_HEARTBEAT_REQ";
        misc::LowHeartBeatReq req;
        if (req.ParseFromString(jmsg.body())){
            Event evt;
            evt.set_src(jmsg.head().dst());
            evt.set_dst(jmsg.head().src());

            misc::LowHeartBeatRsp rsp;
            rsp.set_req_id(req.req_id());
            rsp.set_req_time(req.req_time());
            rsp.set_rsp_time(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());
            evt.set_content(rsp);

            do_send(evt.m_msg);
        }
    } else {
        // FIXME: should keep identity same with head.src
        assert(memcmp(jmsg.head().src().c_str(), identity.data(), identity.size()) == 0);

        m_cb->on_request(jmsg);
        //LOG(ERROR) << "wrong target: myself " << spi->GetCompId() << " target " << jmsg.head().dst().c_str();
    }
}

void ServerImpl::process_send(zmq::socket_t* sock)
{
    Msg* p;
    zmq::message_t msg;
    if (sock->recv(&msg)){
        assert(msg.size() == sizeof(p));
        memcpy(&p, msg.data(), sizeof(p));
        do_send(*p);
        delete p;
    }
}

void ServerImpl::worker_run()
{
    zmq::socket_t* pull_sock = new zmq::socket_t(*msgbus::get_ctx(), ZMQ_PULL);
    pull_sock->connect(m_queue_addr.c_str());

    while (true) {

        try {
            zmq_pollitem_t items[] = {
                { *pull_sock, 0, ZMQ_POLLIN, 0 },
                { *m_worker_sock, 0, ZMQ_POLLIN, 0 },
            };

            int rc = zmq_poll(items, 2, 100);

            if (items[0].revents & ZMQ_POLLIN)
                process_send(pull_sock);

            if (items[1].revents & ZMQ_POLLIN)
                process_request(m_worker_sock);

        } catch(zmq::error_t& e){
            LOG(ERROR) << e.what();
        }

    }
}

void ServerImpl::send_response(const jzs::msg::Msg& msg)
{
    // 必须加锁， m_queue_sock 不支持多线程
    std::unique_lock<std::mutex> lock(m_sending_mutex);

    Msg* m = new Msg();
    *m = msg;
    m->mutable_head()->set_src(this->m_id);

    zmq::message_t zmsg(sizeof(m));
    memcpy(zmsg.data(), &m, sizeof(m));
    m_queue_sock->send(zmsg);
}

void ServerImpl::do_send(const Msg& msg)
{
    try {
        string session = msg.head().dst();
        string buf = msg.SerializeAsString();
        m_worker_sock->send(session.c_str(), session.size(), ZMQ_SNDMORE);
        m_worker_sock->send(buf.c_str(), buf.size());
    } catch (zmq::error_t& e){
        LOG(ERROR) << e.what();
    }
}

