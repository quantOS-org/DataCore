
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
#ifndef JZS_MSGSUBAPI_H
#define    JZS_MSGSUBAPI_H

#include "msgbus/Common.h"

namespace jzs {  
  
    namespace msgbus {
        using ::jzs::msg::Msg;

        using jzs::msg::Msg;
        using jzs::msg::MsgType;

        struct Suber {
            jzs::msg::MsgType tid;
            //int count; //receive count
            //int seqno; // last received sequence no
        };


        class SubscriberCallback {
        public:
            virtual void on_topic(const Msg& cont) = 0;    
        };
  

        class Subscriber {
        public:
            static Subscriber* create(const char *addr, SubscriberCallback* cb);
        
            virtual ~Subscriber() { }

            virtual void subscribe(Suber& sub) = 0;
    
            virtual void start() = 0;
            
            virtual void stop() = 0;
        };
            
  };

}

#endif    

