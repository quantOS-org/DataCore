
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
#ifndef JZS_MSGPUBAPI_H
#define JZS_MSGPUBAPI_H

#include "msgbus/Common.h"

namespace jzs {

    namespace msgbus {

        class Publisher
        {
        public:
            static Publisher* create(const char* addr);

            virtual ~Publisher() {};

            virtual void publish(const jzs::msg::Msg& msg) = 0;

            virtual void start() = 0;

            virtual void stop() = 0;
        };
    }

}

#endif
