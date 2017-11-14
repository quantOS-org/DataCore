
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
#ifndef JZS_ZMDSUBSCRIBER_H
#define JZS_ZMDSUBSCRIBER_H

#include <thread>
#include <zmq.hpp>
#include "Subscriber.h"


namespace jzs{
  
    namespace msgbus {
        using namespace std;
  
        class SubscriberImpl: public Subscriber{
        public:
            SubscriberImpl(const char *addr, SubscriberCallback* cb);

            virtual ~SubscriberImpl();

            virtual void subscribe(Suber& sub) override;

            virtual void start() override;

            virtual void stop() override;

        private:
            void run();
    
    
        private:
            SubscriberCallback* m_cb;
            volatile bool   m_should_exit;
            zmq::socket_t*  m_sock;
            thread *        m_thd;
    
        };
    }
}


#endif
