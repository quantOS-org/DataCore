
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
#ifndef JZS_MSGPUBAPIIMPL_H
#define    JZS_MSGPUBAPIIMPL_H

#include <thread>
#include "Publisher.h"
#include "MsgQueue.h"
#include "zmq.hpp"

namespace jzs {
    namespace msgbus {

        using ::jzs::msg::Msg;

        class PublisherImpl : public Publisher
        {
        public:
            PublisherImpl(const char* addr);

            virtual ~PublisherImpl();

            virtual void publish(const jzs::msg::Msg& msg) override;

            virtual void start() override;

            virtual void stop() override;

        private:
            void run();

        private:
            volatile bool  m_should_exit;
            MsgQueue<Msg>  m_que;
            zmq::socket_t* m_sock;
            std::thread *  m_thd;

        };
    }
}

#endif    /* ZMDPUBLISHER_H */

