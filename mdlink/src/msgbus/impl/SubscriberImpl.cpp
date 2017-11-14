
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
#include "base/Logger.h"
#include "SubscriberImpl.h"
#include "MsgZmqCtx.h"

#include "zmq.h"
#include <cstring>
#include <cstdlib>

using namespace jzs::msgbus;

Subscriber* Subscriber::create(const char *addr, SubscriberCallback* cb)
{
    return new SubscriberImpl(addr, cb);
}

SubscriberImpl::SubscriberImpl(const char *addr, SubscriberCallback* cb):
    m_cb(cb),
    m_should_exit(false),
    m_thd(nullptr)
{
    try {
        m_sock = new zmq::socket_t(*msgbus::get_ctx(), ZMQ_SUB);
        m_sock->connect(addr);
    } catch (exception& e) {
        LOG(FATAL) << e.what();
    }
}

SubscriberImpl::~SubscriberImpl()
{
    if (m_thd) delete m_thd;
    if (m_sock) delete m_sock;
}


void SubscriberImpl::subscribe(Suber& suber)
{
    try {
        uint32_t tid = suber.tid;
        m_sock->setsockopt(ZMQ_SUBSCRIBE, &tid, sizeof(tid));
    } catch (exception& e) {
        LOG(FATAL) << e.what();
    }
}

void SubscriberImpl::start()
{
    m_thd = new thread(std::bind(&SubscriberImpl::run, this));
}

void SubscriberImpl::stop()
{
    if (m_thd) {
        m_should_exit = true;
        m_thd->join();
        delete m_thd;
        m_thd = nullptr;
    }
}

void SubscriberImpl::run()
{
    if (!m_cb) return;

    while (!m_should_exit) {
        try {
            zmq::message_t zmsg;
            if (m_sock->recv(&zmsg, 0)) {
                char* pbuf = (char*)zmsg.data();
                Msg jmsg;
                if (jmsg.ParseFromArray(pbuf + 4, zmsg.size() - 4))
                    m_cb->on_topic(jmsg);
            }
        } catch (exception& e){
            LOG_EVERY_N(ERROR, 10) << e.what();
        }
    }
}
