
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
#ifndef _JZS_QmsClient_H
#define _JZS_QmsClient_H

#include <memory>
#include <string>
#include "protocol/cpp/jzs.h"

namespace jzs {

using namespace jzs::msg::qms;
using namespace jzs::msg::md;
    
class QmsClient {

public:
    QmsClient();

    ~QmsClient();

    bool connect(const std::string& addr);
    
    std::shared_ptr<jzs::msg::qms::MarketQuoteRsp> getMarketData(const std::string& code);
    std::shared_ptr<jzs::msg::qms::Bar1MRsp>       getBar1M(const std::string& code);
    std::shared_ptr<jzs::msg::qms::StrategyMarketQuotesRsp> getStrategyQuotes(uint32_t strategy_id);
    std::shared_ptr<jzs::msg::qms::StrategySubscribeRsp> strategySubsribe(uint32_t strategy_id, std::vector<std::string>& symbols);

private:    
    template <class T>
    void send(int id, const T& req);

    template <class T>
    bool expect(int id, T & rsp);

    bool reconnect();

private:
    void*        m_zmqctx;
    void*       m_sock;
    std::string m_addr;
};

}

#endif
