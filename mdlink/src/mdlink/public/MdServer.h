
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
#ifndef JZS_MDSERVER_H
#define JZS_MDSERVER_H

#include "config/SysConfig.h"
#include "msgbus/Publisher.h"
#include "../public/MapTables.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <exception>

namespace jzs
{    
    class Calendar;
    class MapTables;

    class MdServer
    {
    public: 

        MdServer() :
            m_trade_date(-1),
            m_time_tol(0),
            m_pub(nullptr),
            m_id(""),
            m_calendar(nullptr),
            m_maptables(nullptr)
        {}

        virtual ~MdServer();  
        
        bool init(MdlinkCfg& mdcfg, bool do_publish = true);    
        void StartMdlink(); 
        void ShowStatus();
        MapTables* get_maptables() { return m_maptables; } 

        void set_vwap(MarketQuote& bk) {
            int jzcode = bk.jzcode();
            Instrument inst = m_maptables->get_instrument(jzcode);
            int multiplier = 0;
            if (inst.init)
                multiplier = inst.multiplier;             
            if (bk.volume() != 0 && multiplier != 0) {
                double vwap = bk.turnover() / bk.volume() / multiplier;
                bk.set_vwap(vwap);
            }
            else {
                if (bk.qs().presettle() > 0.0000001)
                    bk.set_vwap(bk.qs().presettle());
                else bk.set_vwap(bk.qs().preclose());
            }
        }
    protected:          
        bool do_publish(jzs::msg::md::MarketDataType type,
                        const jzs::msg::md::MarketQuote& quote, uint32_t jzcode);        
        bool quotes_equal(const jzs::msg::md::MarketQuote& quote,
                          const jzs::msg::md::MarketQuote& last_quote);
        bool quote_valid(const jzs::msg::md::MarketQuote& quote);

        int  inst2jzcode(string inst, string mkt);

    protected:
        virtual void Start() = 0;
        virtual bool init_by_type(MdlinkCfg& mdcfg) = 0;
        virtual void publish(jzs::msg::md::MarketDataType type,
                             const jzs::msg::md::MarketQuote& bk);
        virtual void resubscribe() { ; }

    protected:
        int m_trade_date;
        MillisTime m_time_tol;
        string m_id;
        msgbus::Publisher* m_pub;
        Calendar* m_calendar;
        MapTables* m_maptables;
  };

 
}

#endif