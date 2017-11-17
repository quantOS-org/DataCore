
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
#ifndef _QMS_H
#define _QMS_H

#include <map>
#include <memory>
#include <vector>
#include <stdint.h>
#include <set>
namespace jzs {

using namespace std;
class Calendar;
struct MarketInfo;

struct UniverseCode {
    uint32_t jzcode;
    string symbol;
    bool subscribed;

    UniverseCode(uint32_t a_jzcode, const string& a_symbol, bool a_subscribed) :
        jzcode(a_jzcode),
        symbol(a_symbol),
        subscribed(a_subscribed)
    {}
};

struct UniverseData {
    vector<UniverseCode> codes;
    map<uint32_t, MarketInfo*> markets;

    void append(uint32_t jzcode, const string& symbol, bool a_subscribed);
};

//extern map< uint32_t, shared_ptr<UniverseData> >  g_universe_map;
//extern GlobalTables g_tables;

struct JzCodeInfo {
    string symbol;
    uint32_t mktid;
};

struct SymbolInfo {
    uint32_t jzcode;
    uint32_t mktid;
};

struct InstInfo {
    uint32_t jzcode;
    string symbol;
    uint32_t insttype;
    string instcode;
};

struct AuctTime {
    vector<int> auct_begin_times;
    vector<int> auct_end_times;

    void add_aucttime(int begin_millis, int end_millis) {
        auct_begin_times.push_back(begin_millis);
        auct_end_times.push_back(end_millis);
    }

    bool is_aucttime(int time, int jzcode = -1) {
        for (int i = 0; i < auct_begin_times.size(); i++) {
            if (TimeUtil::compare_time(time, auct_begin_times[i]) < 0)
                continue;
            if (TimeUtil::compare_time(time, auct_end_times[i]) < 0)
                return true;
        }
        return false;
    }
};

struct MarketInfo{
    uint32_t mktid;
    string   code;

    AuctTime auct_time;
    map<uint32_t, AuctTime> special_auct_time;
    int begin_time; //millis
    MillisTime  last_time;
    MillisTime  last_arrived_time;
    int last_date;
    MillisTime  last_bartime;
    int last_bardate;

    MarketInfo(uint32_t a_id, const string& a_code) :
        mktid(a_id),
        code(a_code),
        begin_time(-1),
        last_time(0),
        last_arrived_time(0),
        last_date(0),
        last_bartime(0),
        last_bardate(-1)    
    {}

    // Timeformat MillisTime
    void add_aucttime(int begin_millis, int end_millis) {  
        auct_time.add_aucttime(begin_millis, end_millis);        
        if (begin_time < 0) {
            // uninitialized begin_time
            begin_time = begin_millis;
        }
        else {            
            if (TimeUtil::compare_time(begin_millis, begin_time) < 0) {
                // earlier auct_begin time added
                begin_time = begin_millis;
            }
        }
    }

    void add_special_aucttime(uint32_t jzcode, const AuctTime* at) {
        special_auct_time[jzcode] = *at;
    }

    bool is_aucttime(int time, uint32_t jzcode) {
        auto special = special_auct_time.find(jzcode);
        if (special != special_auct_time.end()) {
            return special->second.is_aucttime(time);
        }
        else {
            return auct_time.is_aucttime(time);
        }
    }

    MillisTime get_begin_time(uint32_t jzcode) {
        auto special = special_auct_time.find(jzcode);
        if (special != special_auct_time.end()) {
            MillisTime begin = special->second.auct_begin_times.front();
            for (auto it = special->second.auct_begin_times.begin();
                it != special->second.auct_begin_times.end(); it++) {
                if (TimeUtil::compare_time(begin, *it) > 0) {
                    begin = *it;
                }
            }
            return begin;
        }
        else {
            return begin_time;
        }
    }

};

class GlobalTables {
public:
    map< string, SymbolInfo>     g_symbol_map;
    map< uint32_t, JzCodeInfo>   g_jzcode_map;   
    map< uint32_t, vector<InstInfo>> g_mkt_inst_map;
    Calendar* g_calendar;

    GlobalTables() {      
    }
    bool init_data();
};

extern string* g_markets;
extern GlobalTables* g_tables;
extern map< uint32_t, MarketInfo*>  g_market_map;
extern map< uint32_t, shared_ptr<UniverseData> >  g_universe_map;
extern int bar_period; // milliseconds
extern int bar_offset; // milliseconds
inline uint32_t jz_code(const string& symbol)
{
    auto it = g_tables->g_symbol_map.find(symbol);
    return (it != g_tables->g_symbol_map.end() ? it->second.jzcode : 0);
}

inline string jz_symbol(int jzcode)
{
    auto iter = g_tables->g_jzcode_map.find(jzcode);
    return (iter != g_tables->g_jzcode_map.end() ? iter->second.symbol : "");
}

}


#endif
