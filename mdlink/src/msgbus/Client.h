
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
 * File:   MsgClientApi.h
 * Author: dxb
 *
 * Created on June 28, 2015, 10:18 PM
 */

#ifndef JZS_MSGCLIENTAPI_H
#define    JZS_MSGCLIENTAPI_H

#include "msgbus/Common.h"

namespace jzs {

    namespace msgbus {

        class ClientCallback
        {
        public:
            virtual void on_response(const jzs::msg::Msg& msg) = 0;
        };


        class Client
        {
        public:
            static Client* create(const char *addr, const char* clientId, ClientCallback* callback);

            virtual ~Client()  { }

            virtual void send_request(const jzs::msg::Msg& msg) = 0;

            virtual void start() = 0;

            virtual bool stop() = 0;

        };
    }

}



#endif    /* MSGCLIENTAPI_H */

