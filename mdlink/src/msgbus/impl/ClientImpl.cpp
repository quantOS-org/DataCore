
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
#include "ClientImpl.h"
#include "Event.h"
#include "MsgZmqCtx.h"

using namespace std;
using namespace std::chrono;
using namespace jzs::msg;

using namespace jzs::msgbus;

Client* Client::create(const char *remote_addr, const char* clientId, ClientCallback* cb)
{
    return new ClientImpl(remote_addr, clientId, cb);
}

ClientImpl::ClientImpl(const char *addr, const char* clientId, ClientCallback* cb)
    : m_remote_addr(addr),
    m_id(clientId),
    m_cb(cb),
    m_thd(nullptr),
    m_should_exit(false),
    m_queue_sock(nullptr),
    m_remote_sock(nullptr)
{
    char buf[200];
    snprintf(buf, 200, "inproc://client_queue_%s_%d", m_id.c_str(), std::rand());
    //printf("%s\n", buf);
    m_queue_addr = buf;
    m_queue_sock = new zmq::socket_t(*msgbus::get_ctx(), ZMQ_PUSH);
    m_queue_sock->bind(m_queue_addr.c_str());

    connect();
}

void ClientImpl::connect()
{
    LOG(INFO) << "Connect to " << m_remote_addr << endl;

    if (m_remote_sock)
        delete m_remote_sock;

    auto sock = new zmq::socket_t(*msgbus::get_ctx(), ZMQ_DEALER);

    sock->setsockopt(ZMQ_IDENTITY, m_id.c_str(), m_id.size());
    int v = 0;
    sock->setsockopt(ZMQ_LINGER, &v, sizeof(v));
    sock->connect(m_remote_addr.c_str());
    m_remote_sock = sock;
}

ClientImpl::~ClientImpl()
{
    if (m_thd)
        delete m_thd;

    if (m_remote_sock) delete m_remote_sock;
    if (m_queue_sock) delete m_queue_sock;
}

void ClientImpl::start()
{
    if (!m_thd)
        m_thd = new std::thread(std::bind(&ClientImpl::run, this));
}


bool ClientImpl::stop()
{
    if (m_thd) {
        m_should_exit = true;
        m_thd->join();
        delete m_thd;
        m_thd = nullptr;
    }
    return true;
}

void ClientImpl::do_send(const Msg& msg)
{
    if (!m_remote_sock) {
        return;
    }
    try {
        string session = msg.head().dst();
        string buf = msg.SerializeAsString();
        m_remote_sock->send(buf.c_str(), buf.size());
    } catch (zmq::error_t& e){
        LOG(ERROR) << e.what();
    }
}

void ClientImpl::run()
{
    time_point<system_clock> last_recv_time = system_clock::now();

    zmq::socket_t* pull_sock = new zmq::socket_t(*msgbus::get_ctx(), ZMQ_PULL);
    pull_sock->connect(m_queue_addr.c_str());


    bool has_send_heatbeat = false;

    while (!m_should_exit) {

        zmq::pollitem_t items[] = {
            { *pull_sock, 0, ZMQ_POLLIN, 0 },
            { *m_remote_sock, 0, ZMQ_POLLIN, 0 },
        };

        zmq::poll(items, 2, 100);

        if (items[0].revents & ZMQ_POLLIN) {
            Msg* p;
            zmq::message_t msg;
            if (pull_sock->recv(&msg)){
                assert(msg.size() == sizeof(p));
                memcpy(&p, msg.data(), sizeof(p));
                do_send(*p);
                delete p;
            }
        }

        if (items[1].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            m_remote_sock->recv(&msg);
            last_recv_time = system_clock::now();
            has_send_heatbeat = false;

            //LOG(WARNING) << "Recv MSG_LOW_HEARTBEAT_RSP";

            Msg jmsg;
            if (jmsg.ParseFromArray(msg.data(), msg.size())) {
                if (jmsg.head().tid()!= MsgType::MSG_LOW_HEARTBEAT_RSP)
                    m_cb->on_response(jmsg);
            }
        }

        auto now = system_clock::now();
        auto d = now - last_recv_time;
        if (d < seconds(1))
            continue;

        // 如果1秒没有收到数据包，发送心跳，已经发送过心跳，超过2秒，则断开重连
        if (!has_send_heatbeat){

            //LOG(WARNING) << "Send MSG_LOW_HEARTBEAT_REQ";

            Event evt;
            evt.set_src(this->m_id);
            evt.set_dst("route");

            misc::LowHeartBeatReq req;
            req.set_req_id(1);
            req.set_req_time(duration_cast<microseconds>(now.time_since_epoch()).count());
            evt.set_content(req);

            do_send(evt.m_msg);
            has_send_heatbeat = true;
        } else {
            if (d > seconds(2)) {
                connect();
                last_recv_time = now;
                has_send_heatbeat = false;
            }
        }
    }

    delete pull_sock;
}

void ClientImpl::send_request(const jzs::msg::Msg& msg)
{
    Msg* m = new Msg();
    *m = msg;
    m->mutable_head()->set_src(this->m_id);

    zmq::message_t zmsg(sizeof(m));
    memcpy(zmsg.data(), &m, sizeof(m));
    m_queue_sock->send(zmsg);
}
