
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
#ifndef _JZS_EVENTPROCESSOR_H
#define _JZS_EVENTPROCESSOR_H
#include <vector>
#include <memory>
#include <thread>
#include <list>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <unistd.h>

#include "msgbus/MsgQueue.h"
#include "protocol/cpp/jzs.h"

namespace jzs {

    using namespace jzs::msg;
    using namespace std;

    template<typename T> struct EventId { 
        enum { ID=0 }; 
    };

#define DEF_EVENT_ID( _TYPE_, _ID_ ) \
    template<> struct EventId<::jzs::msg:: _TYPE_> { \
        enum {    ID = jzs::msg::MsgType :: _ID_   };   \
    }

        DEF_EVENT_ID(qms::MarketQuoteReq,   MSG_QMS_MARKETQUOTE_REQ);
        DEF_EVENT_ID(qms::MarketQuoteRsp,   MSG_QMS_MARKETQUOTE_RSP);
        DEF_EVENT_ID(qms::Bar1MReq,         MSG_QMS_BAR_1M_REQ);
        DEF_EVENT_ID(qms::Bar1MRsp,         MSG_QMS_BAR_1M_RSP);
        DEF_EVENT_ID(qms::StrategyMarketQuotesReq,  MSG_QMS_STRATEGY_MARKETQUOTES_REQ);
        DEF_EVENT_ID(qms::StrategyMarketQuotesRsp,  MSG_QMS_STRATEGY_MARKETQUOTES_RSP);
        DEF_EVENT_ID(qms::StrategySubscribeReq,     MSG_QMS_STRATEGY_SUBSCRIBE_REQ);
        DEF_EVENT_ID(qms::StrategySubscribeRsp,     MSG_QMS_STRATEGY_SUBSCRIBE_RSP);
        
        DEF_EVENT_ID(md::MarketDataInd,     MSG_MD_MARKETDATA_IND);

    class Event {
    public:
        Event() { }
        Event(const Msg& msg) : m_msg(msg)
        {
        }

        uint32_t id() const {
            return m_msg.head().tid();
        }

        ~Event() {
            
        }

        template<typename T>
        shared_ptr<T> content(T* nouse= nullptr) const{

            static_assert(EventId<T>::ID, "type id is zeor!");

            if (EventId<T>::ID == m_msg.head().tid()){
                T* t = new T();
                t->ParseFromString(m_msg.body());
                return shared_ptr<T>(t);
            } else {
                return nullptr;
            }
        }

        const std::string& src() const {
            return m_msg.head().src();
        }

        const std::string& dst() const{
            return m_msg.head().dst();
        }

        void set_src(const std::string& src) {
            m_msg.mutable_head()->set_src(src);
        }

        void set_dst(const std::string& dst) {
            m_msg.mutable_head()->set_dst(dst);
        }

        template<typename T>
        void set_content(const T& t){
            m_msg.mutable_head()->set_tid(EventId<T>::ID);
            m_msg.set_body(t.SerializeAsString());
        }

        Msg  m_msg;
    };

}


#endif
