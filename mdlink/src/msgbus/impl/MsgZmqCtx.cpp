
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
#include <mutex>

#include "MsgZmqCtx.h"

namespace jzs{

    namespace msgbus {

        using namespace std;

        static mutex mtx;
        static std::shared_ptr<::zmq::context_t> ctx;

        std::shared_ptr<::zmq::context_t> get_ctx()
        {
            if (ctx) {
                return ctx;
            } else {
                unique_lock<mutex> lock(mtx);
                if (ctx) return ctx;

                ctx = std::shared_ptr<::zmq::context_t>(new zmq::context_t(3));
                return ctx;
            }
        }
    }
}
