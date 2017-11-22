
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
#ifndef MAPTABLES_H
#define    MAPTABLES_H
#include <string>
#include <set>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <mutex>
#include "protocol/cpp/md.pb.h"
#include "config/SysConfig.h"
#include "base/Logger.h"
#include "msgbus/Publisher.h"
#include "base/Calendar.h"
using namespace jzs::msg;
using namespace jzs::msg::md;

namespace jzs {

    struct MarketInfo {
        string mkt_name;
        shared_ptr<MarketQuote> latest_quote;    
    };
    struct Instrument{
        bool     init;
        int      jzcode;
        int      multiplier;        
        int      mkt;
        int      buylot;
        int      selllot;
        int      insttype;
        int      target_jzcode;
        double   pricetick;
        string   instcode;
        string   symbol; 
        Instrument():
            init(false)
        {}
    };

    class MapTables
    {
    public:
        MapTables();
        virtual ~MapTables();

        void init();

        Instrument get_instrument(uint32_t jzcode) {
            auto it = g_jzcode_to_instrument.find(jzcode);
            if (it != g_jzcode_to_instrument.end()) {
                return it->second;
            }
            else {
                return Instrument();
            }
        }
        string get_symbol(uint32_t jzcode, string def_value);
        string get_market_name(int mkt) {
            auto it = m_markets.find(mkt);
            if (it != m_markets.end()) {
                return it->second;
            }
            else {
                return "";
            }
        }

        std::map< string, vector<string> > m_mkt_instcodes_map; // mkt -> instcode vector
        std::map< string, uint32_t> m_jzcode_map;       // IF1508.CFE -> int
        std::map< string, uint32_t> m_uppersymbol_jzcode_map;       // UPPER(IF1508.CFE)-> int
        std::map< string, uint32_t> g_ctpcode_map;      // IF1508 -> int
        
        std::map< uint32_t, string> g_ctpsymbol_map;    // int -> IF1508
        std::map< uint32_t, string> g_jzcode_to_symbol; // int -> IF1508.CFE       
        
        std::unordered_map <uint32_t, Instrument> g_jzcode_to_instrument;
        map<string, shared_ptr<MarketInfo>>   g_market_infos;
        unordered_map<uint32_t, shared_ptr<MarketInfo>> g_jzcode_to_mktinfo;

    private:
       
        std::map<int, string> m_markets;
    };        
}
#endif