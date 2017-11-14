
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
 * File:   MsgServerApi.h
 * Author: dxb
 *
 * Created on June 28, 2015, 10:16 PM
 */

#ifndef JZS_MSGSERVERAPI_H
#define    JZS_MSGSERVERAPI_H

#include "msgbus/Common.h"

namespace jzs{  
  
    namespace msgbus {

        class ServerCallback {
        public:
            virtual void on_request(const jzs::msg::Msg& msg) = 0;
        };  
  
  
        class Server{
        public:
            static Server* create(const char* local_addr, const char* id, ServerCallback* cb);

            virtual ~Server()  { }
    
            virtual void start() = 0;

            virtual void stop() = 0;

            virtual void send_response(const jzs::msg::Msg& msg) = 0;
        };
    }
}


#endif    /* MSGSERVERAPI_H */

