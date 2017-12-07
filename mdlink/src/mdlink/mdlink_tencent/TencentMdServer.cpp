#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <limits>
#include <cmath>
#include <cstring>
#include <thread>
#include <chrono>
#include <unordered_map>
#include "base/Logger.h"
#include "base/TimeUtil.h"
#include "base/StringUtil.h"
#include "config/SysConfig.h"
#include "TencentMdServer.h"
#include "base/TypeUtil.h"

using namespace std;
using namespace jzs;
using namespace jzs::msg;
using namespace jzs::msg::md;

const int max_codes_num = 75;
const int quote_period  = 1; //s
const int sleep_time = 200; //ms



bool TencentMdServer::init_by_type(MdlinkCfg& mdcfg)
{
    if (mdcfg.route != "TENCENT") {
        LOG(FATAL) << "Error md type " << mdcfg.route << " (TENCENT expected)";
        return false;
    }
    vector<MdCfg> &mdvec = mdcfg.mdvec;
    if (mdvec.size() != 1) {
        LOG(FATAL) << "More than one tencent sources are given";
        return false;
    }
    m_cfg = mdvec[0];     
    return true;    
}
bool TencentMdServer::is_same_quote(shared_ptr<tencent_api::MarketQuote> md1,
    shared_ptr<tencent_api::MarketQuote> md2)
{
    if (md1->volume != md2->volume) {
        return false;
    }

    if (md1->ask1 != md2->ask1 || 
        md1->bid1 != md2->bid1 ||
        md1->ask_vol1 != md2->ask_vol1 ||
        md1->bid_vol1 != md2->bid_vol1 
        ) {
        return false;
    }

    if (md1->ask2 != md2->ask2 ||
        md1->bid2 != md2->bid2 ||
        md1->ask_vol2 != md2->ask_vol2 ||
        md1->bid_vol2 != md2->bid_vol2
        ) {
        return false;
    }

    if (md1->ask3 != md2->ask3 ||
        md1->bid3 != md2->bid3 ||
        md1->ask_vol3 != md2->ask_vol3 ||
        md1->bid_vol3 != md2->bid_vol3
        ) {
        return false;
    }

    if (md1->ask4 != md2->ask4 ||
        md1->bid4 != md2->bid4 ||
        md1->ask_vol4 != md2->ask_vol4 ||
        md1->bid_vol4 != md2->bid_vol4
        ) {
        return false;
    }

    if (md1->ask5 != md2->ask5 ||
        md1->bid5 != md2->bid5 ||
        md1->ask_vol5 != md2->ask_vol5 ||
        md1->bid_vol5 != md2->bid_vol5
        ) {
        return false;
    }
    return true;
}

bool TencentMdServer::tencent_to_jzs(shared_ptr<tencent_api::MarketQuote> md, MarketQuote* pbk)
{
    if (m_trade_date != md->date) {
        return false;
    }
    auto it_md = m_market_quotes.find(md->code);
    if (it_md != m_market_quotes.end()) {
        if (is_same_quote(it_md->second,md)) {
            return false;
        }
    }
    m_market_quotes[md->code] = md;

    MarketQuote& bk = *pbk;
    auto it = m_tencent_inst_map.find(md->code);
    if (it == m_tencent_inst_map.end()) {
        return false;
    }
  
    uint32_t jzcode = it->second.jzcode;    
    MillisTime t = TimeUtil::TimeToMillis(md->time);
    bk.set_time(t);
    bk.set_jzcode(jzcode);
    bk.set_last(md->last);
    bk.set_open(md->open);
    bk.set_high(md->high);
    bk.set_low(md->low);    

    if (it->second.insttype == 100 && it->second.mkt == 2) {
        // SH Index volume should multiply 100
        bk.set_volume(md->volume * 100);
    } else {
        bk.set_volume(md->volume);
    }
    bk.set_turnover(md->turnover);
    bk.set_close(md->close);
    bk.set_settle(0);
    bk.set_interest(0);
    bk.set_delta(0);
    bk.set_iopv(0);
    bk.set_avgaskpx(0);
    bk.set_avgbidpx(0);
    bk.set_totbidvol(0);
    bk.set_totaskvol(0);
    
    auto qs = bk.mutable_qs();
    qs->set_downlimit(0);
    qs->set_uplimit(0);
    qs->set_date(md->date);
    qs->set_tradeday(md->date);
    qs->set_preclose(md->pre_close);
    qs->set_presettle(0);
    qs->set_preinterest(0);
    qs->set_predelta(0);

    auto ab = bk.mutable_ab();
    ab->add_askprice(md->ask1);
    ab->add_bidprice(md->bid1);
    ab->add_askvolume(md->ask_vol1);
    ab->add_bidvolume(md->bid_vol1);

    if (it->second.insttype != 100) {
        ab->add_askprice(md->ask2);
        ab->add_askprice(md->ask3);
        ab->add_askprice(md->ask4);
        ab->add_askprice(md->ask5);
        ab->add_bidprice(md->bid2);
        ab->add_bidprice(md->bid3);
        ab->add_bidprice(md->bid4);
        ab->add_bidprice(md->bid5);
        ab->add_askvolume(md->ask_vol2);
        ab->add_askvolume(md->ask_vol3);
        ab->add_askvolume(md->ask_vol4);
        ab->add_askvolume(md->ask_vol5);
        ab->add_bidvolume(md->bid_vol2);
        ab->add_bidvolume(md->bid_vol3);
        ab->add_bidvolume(md->bid_vol4);
        ab->add_bidvolume(md->bid_vol5);
    } 
    
    set_vwap(bk);
    return true;    
}

void TencentMdServer::Start()
{
    m_thread = new std::thread(&TencentMdServer::Run, std::ref(*this));
}

void TencentMdServer::Run()
{
#ifdef _WIN32
    {
        WORD ver = MAKEWORD(2, 2);
        WSADATA wdata;
        WSAStartup(ver, &wdata);
    }
#endif
    //"180.153.39.51":7709
    const int mkt_size = 3;
    string tencent_mktcodes[] = { "", "sz", "sh" };
    tencent_api::TencentApi api;
    api.set_url(m_cfg.addr);
    vector<string> mkts;
    vector<string> insttypes;
    split(insttypes, m_cfg.insttypes, ";");
    split(mkts, m_cfg.markets, ";");
    set<string> mkts_set;
    set<int> insttype_set;
    for (const auto& mkt : mkts) {
        mkts_set.insert(mkt);
    }
    for (const auto& type : insttypes) {
        insttype_set.insert(toInt(type));
    }

    for (auto it = m_maptables->g_jzcode_to_instrument.begin();
        it != m_maptables->g_jzcode_to_instrument.end(); it++) {
        Instrument& inst = it->second;
        if (insttype_set.find(inst.insttype) != insttype_set.end() && inst.mkt < mkt_size) {
            string mkt_code = m_maptables->get_market_name(inst.mkt);
            if (mkts_set.find(mkt_code) != mkts_set.end()) {
                string tencent_code = tencent_mktcodes[inst.mkt] + inst.instcode;
                m_tencent_inst_map[tencent_code] = inst;
                m_query_codes.push_back(tencent_code);
            }
        }       
    }  
    
    unordered_map<string, uint32_t> symol_jzcode_map;
    vector< vector<string> > stk_codes;
    stk_codes.push_back(vector<string>());
    int i = 0;
    for (auto c : m_query_codes) {
        if (i >= max_codes_num) {
            stk_codes.push_back(vector<string>());
            i = 0;
        }          
        stk_codes.back().push_back(c);
        i++;
    }
    vector<shared_ptr<tencent_api::MarketQuote>> quotes;
    while (true) {
        auto last_time = system_clock::now();
        for (auto codes : stk_codes) {
            quotes.clear();
            if (api.get_quotes(codes, &quotes)) {
                for (auto q : quotes) {
                    MarketQuote bk;
                    if (tencent_to_jzs(q, &bk)) {
                        publish(MarketDataType::MD_STK_L1, bk);
                    }
                }
            }
        }
        
        auto now = system_clock::now();
        LOG(INFO) << "Query time = " << duration_cast<milliseconds>(now - last_time).count() << endl;
        while (now - last_time < seconds(quote_period)) {
            this_thread::sleep_for(milliseconds(sleep_time));
            now = system_clock::now();
        }
    }

}