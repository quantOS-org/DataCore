
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
#include "MdServer.h"
#include "base/Calendar.h"
#include <thread>
#include <memory>
#include <unistd.h>
#ifdef __linux__
#include "shared_ptr_atomic.h"
#endif

using namespace std;
using namespace jzs;
using namespace jzs::msg;
using namespace jzs::msg::md;

namespace jzs {  
    MdServer::~MdServer()
    {
        delete m_maptables;
        delete m_pub;
    }

    bool MdServer::init(MdlinkCfg& mdcfg, bool do_publish)
    {
        m_calendar = Calendar::getInst();
        m_trade_date = m_calendar->GetTradeDay(TimeUtil::dayMillis());
        m_id = mdcfg.id;
        m_time_tol = static_cast<MillisTime> (mdcfg.time_tol);       

        if (!init_by_type(mdcfg)) {
            return false;
        }     

		m_maptables = new MapTables();
		m_maptables->init();

        if (do_publish) {
            m_pub = msgbus::Publisher::create(mdcfg.addr.c_str());
        }
        else {
            m_pub = nullptr;
        }            
          
        return true;
    }

    int MdServer::inst2jzcode(string inst, string mkt)
    {
        string instcode = inst + "." + mkt;

        auto it = m_maptables->m_jzcode_map.find(instcode);
        if (it != m_maptables->m_jzcode_map.end()) {
            return it->second;
        }
        else {
            return 0;
        }
    }

    void MdServer::StartMdlink()
    {
        // Must start m_pub first!
        m_pub->start();
        // Let client connected firstly
        LOG(INFO) << "Sleep 5s for clients";
        this_thread::sleep_for(chrono::seconds(5));
        Start();
    }

    void MdServer::ShowStatus()
    {                
        LOG(INFO) << "I am alive!";
        for (auto it = m_maptables->g_market_infos.begin(); it != m_maptables->g_market_infos.end(); it++){
            string mkt_name = it->second->mkt_name;
            auto latest_quote = atomic_load(&it->second->latest_quote);
            if (latest_quote)
                LOG(INFO) << "LATEST MARKET DATA: "
                << setw(3) << mkt_name << " "
                << setw(9) << latest_quote->qs().date()<<" "<<TimeUtil::time_to_hhmsMS(latest_quote->time()) << " "
                << m_maptables->g_jzcode_to_symbol[latest_quote->jzcode()] << " "
                << latest_quote->last();
        }        
    }

    bool MdServer::quotes_equal(const jzs::msg::md::MarketQuote& quote,
        const jzs::msg::md::MarketQuote& last_quote)
    {
        if (quote.last() != last_quote.last() ||
            quote.time() != last_quote.time() ||
            quote.volume() != last_quote.volume() ||
            quote.turnover() != last_quote.turnover() ||
            quote.interest() != last_quote.interest() ||
            quote.open() != last_quote.open() ||
            quote.close() != last_quote.close() ||
            quote.high() != last_quote.high() ||
            quote.low() != last_quote.low() ||
            quote.settle() != last_quote.settle() ||
            quote.delta() != last_quote.delta() ||
            quote.iopv() != last_quote.iopv() ||
            quote.avgaskpx() != last_quote.avgaskpx() ||
            quote.avgbidpx() != last_quote.avgbidpx() ||
            quote.totaskvol() != last_quote.totaskvol() ||
            quote.totbidvol() != last_quote.totbidvol()) {
            return false;
        }

        if (quote.ab().askprice_size() != last_quote.ab().askprice_size() ||
            quote.ab().askvolume_size() != last_quote.ab().askvolume().size() ||
            quote.ab().bidprice_size() != last_quote.ab().bidprice_size() ||
            quote.ab().bidvolume_size() != last_quote.ab().bidvolume_size()) {
            return false;
        }

        int depth = quote.ab().askprice_size();
        for (int i = 0; i < depth; i++) {
            if (quote.ab().askprice(i) != last_quote.ab().askprice(i) ||
                quote.ab().askvolume(i) != last_quote.ab().askvolume(i) ||
                quote.ab().bidprice(i) != last_quote.ab().bidprice(i) ||
                quote.ab().bidvolume(i) != last_quote.ab().bidvolume(i)) {
                return false;
            }
        }
        return true;
    }

    // Is it a valid tick. 
    bool MdServer::quote_valid(const jzs::msg::md::MarketQuote& quote)
    {
        if (quote.qs().tradeday() != m_trade_date) {
            return false;
        }
        MillisTime now = TimeUtil::dayMillis();
        if (TimeUtil::compare_time(quote.time(), now) > 0) {
            MillisTime diff;
            if (quote.time() > now) {
                diff = quote.time() - now;
            }
            else {
                diff = now + TimeUtil::millis_per_day - quote.time();
            }
            if (diff > m_time_tol) {
                // quote time should not be later than current time with a threshold check
                return false;
            }
        }
        return true;
    }


    // publish quote using jzcode if jzcode is not zero
    bool MdServer::do_publish(jzs::msg::md::MarketDataType type,
        const jzs::msg::md::MarketQuote& quote, uint32_t jzcode) {
        MarketDataInd ind;
        ind.set_type(type);
        switch (type) {
        case MarketDataType::MD_FUT_L1:
        case MarketDataType::MD_FUT_L2:
            *ind.mutable_fut() = quote;
            if (jzcode) ind.mutable_fut()->set_jzcode(jzcode);
            break;
        case MarketDataType::MD_STK_L1:
        case MarketDataType::MD_STK_L2:
            *ind.mutable_stk() = quote;
            if (jzcode) ind.mutable_stk()->set_jzcode(jzcode);
            break;
        default:
            return false;
        }

        Msg msg;
        msg.mutable_head()->set_tid(MsgType::MSG_MD_MARKETDATA_IND);
        msg.mutable_head()->set_src(m_id);
        msg.mutable_head()->set_dst("all");

        if (ind.SerializeToString(msg.mutable_body())) {
            if (m_pub)
                m_pub->publish(msg);
            return true;
        }
        else {
            printf("Serialize error!\n");
            return false;
        }
    }

    void MdServer::publish(jzs::msg::md::MarketDataType type,
        const jzs::msg::md::MarketQuote& quote_in)
    {
        // 保存时间戳最新的记录
        MarketQuote quote(quote_in);
        // convert to jz date format        
        int trade_day = m_trade_date;
        MillisTime time = TimeUtil::dayMillis();
        int action_day = m_calendar->GetActionDayFromTradeDay(trade_day, quote.time());
        if (quote.time() > TimeUtil::millis_per_day) {
            //错误的tick时间，垃圾tick，直接丢掉
            LOG(ERROR) << "Wrong tick time " << quote.time() << " " << m_maptables->g_jzcode_to_symbol[quote.jzcode()] <<
                " " << quote.last() << " " << quote.volume() << " " << quote.jzcode() << endl;
            return;
        }

        if (quote.mutable_qs()->date() != trade_day &&
            quote.mutable_qs()->date() != action_day) {
            //错误的tick日期，垃圾tick，直接丢掉
            LOG(ERROR) << "Wrong tick date: " << quote.mutable_qs()->date() << " " <<
                TimeUtil::MillisToTime(quote.time()) << " " << m_maptables->g_jzcode_to_symbol[quote.jzcode()] <<
                " " << quote.last() << " " << quote.volume() << " " << quote.jzcode() << endl;
            return;
        }

        else {
            quote.mutable_qs()->set_date(action_day);
            quote.mutable_qs()->set_tradeday(trade_day);
        }

        auto it = m_maptables->g_jzcode_to_mktinfo.find(quote_in.jzcode());
        if (it != m_maptables->g_jzcode_to_mktinfo.end()) {
            auto info = it->second;
            if (!info->latest_quote ||
                info->latest_quote->qs().date() < quote.qs().date() ||
                (info->latest_quote->time() < quote.time() && info->latest_quote->qs().date() == quote.qs().date())) {

                bool ignore = false;
                auto symbol = m_maptables->g_jzcode_to_symbol.find(quote.jzcode());
                if (symbol != m_maptables->g_jzcode_to_symbol.end()) {
                    // 过滤期权行情
                    if (symbol->second.size() == 11 && strstr(symbol->second.c_str(), ".SH"))
                        ignore = true;
                }

                // 简单过滤掉错误行情（包括夜盘行情）, 在当前时间30分钟之后的
                if (!ignore && TimeUtil::compare_time(quote.time(),
                    TimeUtil::time_add(TimeUtil::dayMillis(), 2 * TimeUtil::millis_per_minute)) >= 0)
                    ignore = true;

                if (!ignore) {
                    auto q = shared_ptr<MarketQuote>(new MarketQuote);
                    *q = quote;                    
                    atomic_store(&info->latest_quote, q);
                }
            }
        }
        else {
            LOG(ERROR) << "Jzcode is not found: " << quote.jzcode();
            return;
        }
        do_publish(type, quote, 0);

    }

}
