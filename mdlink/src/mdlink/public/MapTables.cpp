
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
#include "base/Calendar.h"
#include "base/StringUtil.h"
#include "base/Logger.h"
#include "base/Init.h"
#include "MapTables.h"
#include "base/csv-reader.h"
#include <fstream>

using namespace std;
using namespace jzs;
using namespace jzs::msg;
using namespace jzs::msg::md;

namespace jzs {

    MapTables::MapTables()
    {
      
    }

    MapTables::~MapTables()
    {}

    void MapTables::init()
    {
        // load market
        {
            string path = SysConfig::getEtcDir() + "/market.csv";
            CSVReader reader(path);
            reader.LoadFile();

            // skip first 
            // "market","nationcode","timezone","marketcode","auctbeg1","auctend1","auctbeg2","auctend2","auctbeg3","auctend3","auctbeg4","auctend4"
            for (int i = 1; i < reader.get_num_rows(); i++) {
                auto info = shared_ptr<MarketInfo>(new MarketInfo);
                int id = atoi(reader.rows[i][0].c_str());
                string marketcode = reader.rows[i][3];
                m_markets[id] = trim(marketcode);
                string s = trim(marketcode);
                info->mkt_name = s.c_str();
                g_market_infos[info->mkt_name] = info;

            }
        }

        // load instrument
        {
            string path = SysConfig::getEtcDir() + "/instrument.csv";
            CSVReader reader(path);
            reader.LoadFile();
            
            // skip first
            // "instcode","jzcode","targetjzcode","market","buylot","selllot","pricetick","multiplier","insttype","symbol"
            for (int i = 1; i < reader.get_num_rows(); i++) {

                string instcode = trim(reader.rows[i][0].c_str());
                int jzcode = atoi(reader.rows[i][1].c_str());
                int target_jzcode = atoi(reader.rows[i][2].c_str());
                int market = atoi(reader.rows[i][3].c_str());
                int buylot = atoi(reader.rows[i][4].c_str());
                int selllot = atoi(reader.rows[i][5].c_str());
                double pricetick = atof(reader.rows[i][6].c_str());
                int multiplier = atoi(reader.rows[i][7].c_str());
                int insttype = atoi(reader.rows[i][8].c_str());
                string symbol = trim(reader.rows[i][9].c_str());

                Instrument inst;
                inst.init = true;
                inst.multiplier = multiplier;
                inst.jzcode = jzcode;
                inst.mkt = market;
                inst.buylot = buylot;
                inst.selllot = selllot;
                inst.pricetick = pricetick;
                inst.insttype = insttype;
                inst.symbol = symbol;
                inst.target_jzcode = target_jzcode;
                inst.instcode = instcode;

                m_jzcode_map[symbol] = jzcode;
                g_jzcode_to_symbol[jzcode] = symbol;
                g_jzcode_to_instrument[jzcode] = inst;
                vector<string> ss = split(symbol, ".");
                if (ss.size() == 1) {
                    LOG(ERROR) << "Wrong instcode " << symbol;
                    continue;
                }

                string mkt = ss[ss.size() - 1];
                {
                    auto it = g_market_infos.find(mkt);
                    if (it != g_market_infos.end())
                        g_jzcode_to_mktinfo[jzcode] = it->second;
                    else
                        LOG(ERROR) << "Can't find market info for " << mkt;
                }

                if (mkt == "SH" || mkt == "SZ") {
                    m_mkt_instcodes_map[mkt].push_back(ss[0]);
                }

                if (mkt == "SHF" || mkt == "DCE" || mkt == "CZC" || mkt == "CFE")
                {
                    g_ctpsymbol_map[jzcode] = ss[0];
                    g_ctpcode_map[ss[0]] = jzcode;
                    //    LOG(INFO) << "instrument: " << jzcode << " " << instcode;
                }
            }
        }

        for (auto& e : m_jzcode_map) {
            string code = e.first;
            transform(code.begin(), code.end(), code.begin(), ::toupper);
            m_uppersymbol_jzcode_map[code] = e.second;
        }

        LOG(INFO) << "Load " << m_jzcode_map.size() << " symbols";
    }

    string MapTables::get_symbol(uint32_t jzcode, string def_value)
    {
        auto it = g_jzcode_to_symbol.find(jzcode);
        if (it != g_jzcode_to_symbol.end())
            return it->second;
        else
            return def_value;
    }

}