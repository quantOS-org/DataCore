
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
/* 
 * File:   ServerImpl.h
 * Author: dxb
 *
 * Created on June 28, 2015, 10:21 PM
 */

#ifndef JZS_MSGSERVERAPIIMPL_H
#define    JZS_MSGSERVERAPIIMPL_H

#include <vector>
#include <list>
#include <thread>
#include <string>
#include <mutex>
#include <memory>
#include "zmq.hpp"
#include "zhelpers.hpp"
#include "Server.h"

using namespace std;

namespace jzs{
  
    namespace msgbus {
        class ServerImpl: public Server {
        public:
            ServerImpl(const char* local_addr, const char* id, ServerCallback* cb);
    
            virtual ~ServerImpl();
        
            virtual void start() override;
            virtual void stop() override;

            virtual void send_response(const jzs::msg::Msg& msg) override;

        private:
            void worker_run();
            void proxy_run();

            void do_send(const jzs::msg::Msg& msg);

            void process_request(zmq::socket_t* sock);
            void process_send(zmq::socket_t* sock);
        private: 
            string m_id;
            string m_local_addr;
            string m_backend_addr;
            string m_queue_addr;
    
            ServerCallback* m_cb;

            zmq::socket_t* m_frontend_sock;
            zmq::socket_t* m_backend_sock;
            zmq::socket_t* m_worker_sock;
            zmq::socket_t* m_queue_sock;

            std::thread*   m_worker_thread;
            std::thread*   m_proxy_thread;
            std::mutex     m_sending_mutex;
        };
    }  
}

#endif    /* MSGSERVERAPIIMPL_H */

