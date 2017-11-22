
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
#ifndef JZS_TdfMdServer_H
#define JZS_TdfMdServer_H

#include <thread>
#include <string>
#include <set>
#include <map>
#include "protocol/cpp/md.pb.h"
#include "base/Logger.h"
#include "../public/MdServer.h"
#include "TDFAPI.h"

namespace jzs{
    using namespace std;

    using jzs::msg::Msg;

    struct TdfMarketInfo {
        int trade_date;
        string ct_filename;
    };    

    class TdfMdServer : public MdServer {
    public:
        TdfMdServer();
        ~TdfMdServer();
        virtual void Start() override;

    private:

        virtual bool init_by_type(MdlinkCfg& mdcfg);

        void LoadSymbol();
        inline bool IsWanted(const char* windCode);

        void Publish(jzs::msg::md::MarketDataType type, const jzs::msg::md::MarketQuote& bk);
        void Run(); 
        void subscribe(THANDLE hTdf, TDF_CODE_RESULT* codeResult);
        uint32_t tdcode_to_jzcode(const char* tdcode, const TDF_CODE_INFO * pCodeInfo, bool is_index = false);
        bool TdToJZS(const TDF_MARKET_DATA* md, jzs::msg::md::MarketQuote* pbk);
        bool TdToJZS(const TDF_INDEX_DATA* md,  jzs::msg::md::MarketQuote* pbk);
        bool TdToJZS(const TDF_FUTURE_DATA* md, jzs::msg::md::MarketQuote* pbk);
        uint32_t find_jzsymbol_UPPER(const string& symbol);
        bool get_ct_filename(const string& mkt, string* filename, int* tradedate);

        string build_ct_filename(const string& mkt, int tradedate);

    private:
        MdCfg m_cfg;
        std::thread* m_thread;
        map<string, TdfMarketInfo> m_market_infos;

        int m_reqid;
        bool m_shouldExit;
        bool m_has_subscribed;
        volatile int m_data_count;

        chrono::time_point<chrono::system_clock> m_heartbeat_time;

        bool m_subscribed_all;
        std::set<std::string> m_subscribed_jzcodes;

        std::map< string, uint32_t> m_subscribed_tdcode_map;

        
        void OnMarketData(const TDF_MSG* pMsgHead);
        void OnSysMessage(const TDF_MSG* pSysMsg);

        static void RecvData(THANDLE hTdf, TDF_MSG* pMsgHead);
        static void RecvSys(THANDLE hTdf, TDF_MSG* pSysMsg);

        
        volatile int m_today;

        volatile THANDLE m_tdf;

        string m_ct_filename;

    };
}

#endif
 
