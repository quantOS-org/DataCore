
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
#define _WINSOCKAPI_
#include <thread>
#include <memory>
#include "config/SysConfig.h"
#include "QmsData.h"
#include "QmsSvr.h"
#include "Qms.h"
#include "base/Init.h"
#include "base/csv-reader.h"
#include <set>
#include "base/Calendar.h"
using namespace std;
using namespace jzs;

namespace jzs {
    GlobalTables* g_tables = nullptr;
    map< uint32_t, shared_ptr<UniverseData> >  g_universe_map;
    map< uint32_t, MarketInfo*>  g_market_map;
    string* g_markets = nullptr;
    int bar_period = 60000;
    int bar_offset = 3000;
    // strategy_id -> [ jzcode, symbole ]
    void UniverseData::append(uint32_t jzcode, const string& symbol, bool a_subscribed)
    {
        auto it = g_tables->g_jzcode_map.find(jzcode);
        if (it == g_tables->g_jzcode_map.end())
            return;
        auto& info = it->second;

        this->codes.push_back(UniverseCode(jzcode, symbol, a_subscribed));
        if (this->markets.find(info.mktid) == this->markets.end()){
            this->markets[info.mktid] = g_market_map[info.mktid];
        }
    }

    void init_data()
    {
        g_tables = new GlobalTables();
        g_tables->init_data();

		MarketInfo * cfe_mkt = NULL;

        // load market
        int trading_day = g_tables->g_calendar->GetTradeDay(TimeUtil::dayMillis());

		string path = SysConfig::getEtcDir() + "/market.csv";
		CSVReader reader(path);
		reader.LoadFile();

		// skip first 
		// "market","nationcode","timezone","marketcode","auctbeg1","auctend1","auctbeg2","auctend2","auctbeg3","auctend3","auctbeg4","auctend4"
		for (int i = 1; i < reader.get_num_rows(); i++) {

			int marketid = atoi(reader.rows[i][0].c_str());
            if (marketid == 0) {
                LOG(ERROR) << "Market id is 0";
                continue;
            }

			string s = trim(reader.rows[i][3].c_str());
            string marketcode = s.c_str();

            if (marketcode == "") {
                LOG(ERROR) << "Market code is empty";
                continue;
            }

            MarketInfo* tmp = new MarketInfo(marketid, marketcode);

            char auctbeg[9] = "auctbeg_";
            char auctend[9] = "auctend_";
           
            bool time_frm_set = false;

			int offset = 4;
            for (int i = 1; i <= 4; i++){
				
				int index = offset + (i - 1) * 2;
				int auctbeg_t = atoi(reader.rows[i][index].c_str()) * 100 * 1000;
				int auctend_t = atoi(reader.rows[i][index+1].c_str()) * 100 * 1000;
                auctbeg_t = TimeUtil::TimeToMillis(auctbeg_t);
                auctend_t = TimeUtil::TimeToMillis(auctend_t);
                //if (TimeUtil::isInNightTime)
                if (TimeUtil::compare_time(auctbeg_t, auctend_t) < 0){
                    time_frm_set = true;
                    tmp->add_aucttime(auctbeg_t, auctend_t);
                }                
            }

            if (marketcode == "CFE") {
                cfe_mkt = tmp;
            }

            if (marketcode == "SHF" || marketcode == "DCE" ||
                marketcode == "CZC") {
                //21:00->2:30
                tmp->add_aucttime(TimeUtil::TimeToMillis(210000 * 1000), TimeUtil::TimeToMillis(23000 * 1000));
            }

            g_market_map[tmp->mktid] = tmp;
        }
        LOG(INFO) << "Market loading finished!";

        if (cfe_mkt != nullptr) {
            int mktid = cfe_mkt->mktid;
            AuctTime at;
            at.add_aucttime(TimeUtil::TimeToMillis( 93000 * 1000), TimeUtil::TimeToMillis(113000 * 1000));
            at.add_aucttime(TimeUtil::TimeToMillis(130000 * 1000), TimeUtil::TimeToMillis(150000 * 1000));
			auto it = g_tables->g_mkt_inst_map.find(mktid);
			if (it != g_tables->g_mkt_inst_map.end()) {
				for (auto it_inst = it->second.begin(); it_inst != it->second.end(); it_inst++) {
					if (it_inst->insttype == 101) { // INDEX FUTURE
						cfe_mkt->add_special_aucttime(it_inst->jzcode, &at);
					}
				}
			}    
        }
        g_markets = new string[g_market_map.size()];
        // mkt code 
        // attention: the index shift -1, it means 0->"SZ", 1->"SH", ...
        for (auto it = g_market_map.begin(); it != g_market_map.end(); it++) {
            g_markets[it->first - 1] = it->second->code;
        }

    }

    bool GlobalTables::init_data()
    {
        g_calendar = Calendar::getInst();
        // 下面的代码修改成直接读取数据库
        int trading_day = g_calendar->GetTradeDay(TimeUtil::dayMillis());

		string path = SysConfig::getEtcDir() + "/instrument.csv";
		CSVReader reader(path);
		reader.LoadFile();

		// skip first
		// "instcode","jzcode","targetjzcode","market","buylot","selllot","pricetick","multiplier","insttype","symbol"
		for (int i = 1; i < reader.get_num_rows(); i++) {

			string instcode = trim(reader.rows[i][0].c_str());
			int jzcode = atoi(reader.rows[i][1].c_str());
			int target_jzcode = atoi(reader.rows[i][2].c_str());
			int mktid = atoi(reader.rows[i][3].c_str());
			int buylot = atoi(reader.rows[i][4].c_str());
			int selllot = atoi(reader.rows[i][5].c_str());
			double pricetick = atof(reader.rows[i][6].c_str());
			int multiplier = atoi(reader.rows[i][7].c_str());
			int insttype = atoi(reader.rows[i][8].c_str());
			string symbol = trim(reader.rows[i][9].c_str());

            JzCodeInfo codeInfo;
            SymbolInfo symInfo;
			InstInfo instInfo;

            codeInfo.mktid = mktid;
            codeInfo.symbol = symbol;

			symInfo.mktid = mktid;
            symInfo.jzcode = jzcode;

			instInfo.instcode = instcode;
			instInfo.symbol = symbol;
			instInfo.jzcode = jzcode;
			instInfo.insttype = insttype;

			g_symbol_map[symbol] = symInfo;
            g_jzcode_map[jzcode] = codeInfo;
			auto it = g_mkt_inst_map.find(mktid);
			if (it != g_mkt_inst_map.end()) {
				it->second.push_back(instInfo);
			}
			else {
				vector<InstInfo> infos;
				infos.push_back(instInfo);
				g_mkt_inst_map[mktid] = infos;
			}			
        }

        LOG(INFO) << "Load " << g_jzcode_map.size() << " symbols";       

        return true;
    }

}

// 在入口函数一开始添加以下代码  

int QmsMain(int argc, char* argv[])
{
    string  qmsid;
    if (argc == 2) {
        qmsid = string(argv[1]);
        init(argv[1]);
    }
    else {
        init(argv[0]);
        LOG(ERROR) << "Wrong number of arguments given!" << endl;
        return -1;
    }   

    init_data();   

    QmsCfg cfg;

    jzs::QmsData* qmsdata = jzs::QmsData::instance();

    jzs::QmsSvr*  qmssvr = new jzs::QmsSvr();

    bar_period = SysConfig::getQmsCfg()->bar_type * 1000;
    bar_offset = SysConfig::getQmsCfg()->bar_offset * 1000;

    qmsdata->Init();

    qmssvr->init();

    qmsdata->Start();

    qmssvr->run();

    return 0;

}
