
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
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include "base/Logger.h"
#include "MsgZmqCtx.h"
#include "zmq.h"

#include "PublisherImpl.h"


using namespace std;
using namespace jzs::msgbus;

Publisher* Publisher::create(const char* addr)
{
    return new PublisherImpl(addr);
}

PublisherImpl::PublisherImpl(const char* addr) :
    m_thd(nullptr),
    m_should_exit(false)
{
    LOG(INFO) << "MsgPub addr: " << addr;

    auto ctx = msgbus::get_ctx();

    m_sock = new zmq::socket_t(*ctx, ZMQ_PUB);
    m_sock->bind(addr);
}

PublisherImpl::~PublisherImpl()
{
    delete m_sock;
    delete m_thd;
}

void PublisherImpl::publish(const Msg& cont)
{
    while( !m_que.Push(cont) ) {
        LOG(ERROR) << "PublisherImpl: queue is full";
    }
}

void PublisherImpl::start()
{
    m_thd = new std::thread(&PublisherImpl::run, std::ref(*this));
}

void PublisherImpl::stop()
{
    m_should_exit = true;
    m_thd->join();
}

void PublisherImpl::run()
{
    Msg jmsg;
    
    while(!m_should_exit) {
        bool ret = m_que.Pop(jmsg, 10);
        if( !ret )
            continue;

        try {
            int len = jmsg.ByteSize() + 4;
            zmq::message_t zmsg(len);
            char* buf = (char*)zmsg.data();

            uint32_t topic = jmsg.head().tid();
            memcpy(buf, &topic, sizeof(uint32_t));

            if (!jmsg.SerializeToArray(buf + 4, len-4)) {
                LOG_EVERY_N(ERROR, 10) << "Can't SerializeToArray";
                continue;
            }
            
            if (!m_sock->send(zmsg, 0)) {
                LOG_EVERY_N(ERROR, 10) << "send failure";
            }

        } catch (exception& e) {
            LOG_EVERY_N(ERROR, 10) << e.what();
        }
    }
}
