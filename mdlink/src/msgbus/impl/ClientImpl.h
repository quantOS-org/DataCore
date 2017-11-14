
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
 * File:   MsgClientApiImpl.h
 * Author: dxb
 *
 * Created on June 28, 2015, 10:20 PM
 */

#ifndef JZS_MSGCLIENTAPIIMPL_H
#define    JZS_MSGCLIENTAPIIMPL_H

#include <string>
#include <thread>
#include <mutex>
#include <memory>
#include <list>

#include "Client.h"
#include "zmq.hpp"
#include "zhelpers.hpp"

namespace jzs{
  
    namespace msgbus{

        using namespace std;
  
        class ClientImpl: public Client{
        public:
            ClientImpl(const char *remote_addr, const char* clientId, ClientCallback* callback);

            virtual ~ClientImpl();
    
            virtual void send_request(const jzs::msg::Msg& msg) override;

            virtual void start() override;

            virtual bool stop() override;
    
        private:
            void run();

            void connect();
    
            inline void do_send(const jzs::msg::Msg& msg);
        private:
            string m_id;
            string m_remote_addr;
            string m_queue_addr;
            zmq::socket_t*          m_queue_sock;
            zmq::socket_t*          m_remote_sock;
            ClientCallback *        m_cb;
            std::thread *           m_thd;
            volatile bool           m_should_exit;
        };
    }
}

#endif    /* MSGCLIENTAPIIMPL_H */

