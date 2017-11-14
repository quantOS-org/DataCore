
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
#ifndef _JZS_QMSSVR_H
#define _JZS_QMSSVR_H

#include <cstdint>
#include <string>
#include <list>
#include <map>
#include <memory>
#include <stdio.h>
#include <zmq.hpp>
#include <thread>

#include "base/Logger.h"
#include "config/SysConfig.h"
#include "protocol/cpp/qms.pb.h"

namespace jzs{
  
    using namespace std;
    using namespace std::chrono;
    
    class QmsSvr;

    class WorkerInfo;

    class QmsSvr_Worker {
        friend class QmsSvr;
    public:

        QmsSvr_Worker(zmq::context_t* ctx, int id);

        void onHeartBeatReq(const char* data, size_t size);
        void onMaketQuoteReq(const char* data, size_t size);
        void onStrategyMarketQuotesReq(const char* data, size_t size);
        void onStrategySubscribeReq(const char* data, size_t size);
        void onBar1MReq(const char* data, size_t size);

        void sendRsp(uint32_t type, const google::protobuf::Message& rsp);

        void run();

    private:
        zmq::socket_t*  m_sock;
        int m_id;
        zmq::message_t m_request_msg;
        zmq::message_t m_addr_msg;
        zmq::message_t m_empty_msg;
        zmq::context_t* m_ctx;
        volatile bool   m_should_exit;
        thread*         m_thread;
    };


    class QmsSvr {
        friend class QmsSvr_Worker;
    public:

        QmsSvr();

        void init();
        
        void run();

        void send_heartbeat_req(WorkerInfo*);
        void forward_request(WorkerInfo*);
        shared_ptr<WorkerInfo> forward_reply();


        inline void check_status();

        inline void forward_data();

    private:
        string          m_addr;
        string          m_id;
        zmq::context_t* m_ctx;
        zmq::socket_t*  m_frontend;
        zmq::socket_t*  m_backend;
        system_clock::time_point m_last_worker_time;
        system_clock::time_point m_last_client_time;
        std::list<shared_ptr<WorkerInfo>> m_worker_queue;
    };
   
    
};

#endif
