
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
#ifndef JZS_CTPMDSERVER_H
#define JZS_CTPMDSERVER_H

#include <vector>
#include <string>

#include "ThostFtdcMdApi.h"
#include "base/Logger.h"
#include "../public/MdServer.h"
#include "protocol/cpp/md.pb.h"

namespace jzs
{

    using namespace std;
    using namespace ::jzs::msg;

    class CtpMdServer : public MdServer, public CThostFtdcMdSpi
    {
    private:
        int reqid;
        CThostFtdcMdApi* userapi;
        MdCfg m_cfg;
        std::vector<std::string> m_subscribed;

    public:
        CtpMdServer();

        ~CtpMdServer();

        void Run();

    private:

        virtual void Start();
        virtual bool init_by_type(MdlinkCfg& mdcfg);
        virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo,
            int nRequestID, bool bIsLast);

        virtual void OnFrontDisconnected(int nReason);

        virtual void OnHeartBeatWarning(int nTimeLapse);

        virtual void OnFrontConnected();

        virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);
        void resubscribe();

    private:
        void PublishMarketQuote(jzs::msg::md::MarketDataType type, const jzs::msg::md::MarketQuote* bk);

        inline bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);

        void LoadSymbol();
        int ctp_jzcode(const char* mkt, const char* symbol);
        string ctp_symbol(int jzcode);
        inline double Norm(double price);
    };
}

#endif
