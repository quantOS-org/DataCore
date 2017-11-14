
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
#ifndef _JZS_QMS_DATA_H
#define _JZS_QMS_DATA_H

#include <cstdint>
#include <string>
#include <list>
#include <map>
#include <stdio.h>
#include <zmq.h>
#include <thread>
#include <mutex>
#include <unordered_map>

#include "base/Logger.h"
#include "base/TimeUtil.h"
#include "config/SysConfig.h"
#include "msgbus/Subscriber.h"
#include "protocol/cpp/jzs.h"
#include "Qms.h"
#include <memory>

extern bool g_verbose;
extern bool g_replay;

namespace jzs{
  
    using namespace ::std;
    using namespace ::jzs::msg;
    using namespace ::jzs::msg::md;
    typedef uint32_t MktId;
    struct QuoteData;
    
    struct Bar {
        shared_ptr<MarketQuote> last;
        int32_t   date;
        int32_t   bar_time;
        double    open;
        double    high;
        double    low;
        double    close;
        double    vwap;
        double    volume_total;
        double    turnover_total;
        int32_t   match_item;
        int64_t   interest_total;
        int32_t   flag;
        bool      is_auct;
        double    volume_inc;
        double    turnover_inc;
        int64_t   interest_inc;
        enum BarFlag {
            NoFlag = 0x0,
            AutoCompleted = 0x1, 
            HasTurnover = 0x2, // The first quote having turnover will decide the open price of the bar. 
            HasQuote = 0x4   // If the flag is false, last quote is invalid, don't use it.
        };
        bool hasFlag(BarFlag bit_flag)
        {
            return flag & bit_flag;
        }
        void setFlag(BarFlag bit_flag)
        {
            flag |= bit_flag;
        }
        void clearFlag(BarFlag bit_flag)
        {
            flag &= ~bit_flag;
        }
        void cleanFlag()
        {
            flag = BarFlag::NoFlag;
        }
        Bar() :
            last(nullptr),date(0), bar_time(0),
            open(0), high(0), low(0), close(0), vwap(0.0),
            volume_total(0), turnover_total(0), match_item(0), interest_total(0), flag(0),
            is_auct(false),
            volume_inc(0), turnover_inc(0), interest_inc(0)
        {

        }
        
    };

    struct QuoteData {
        MarketQuote         last;
        vector<jzs::Bar>    bar_1m;
        MillisTime          last_bartime;
        int                 last_bardate;
        MillisTime          last_arrived_time;
        MarketInfo*         mkt;
        bool                initialized;
        
        QuoteData() :
            last_bartime(0),
            last_bardate(-1),
            last_arrived_time(0),
            mkt(nullptr),
            initialized(false)
        {}
        /*
        QuoteData(const MarketQuote* quote, MarketInfo* a_mkt) :
            last(*quote),
            last_bartime(0),
            last_arrived_time(0),
            mkt(a_mkt),
            initialized(false)
        {}
        */
        ~QuoteData() {
        }
    };


    class QmsSvr;
    class QmsSvr_Worker;
    
    class QmsData : public msgbus::SubscriberCallback {
        friend class QmsSvr;
        friend class QmsSvr_Worker;
    public:
        QmsData() :m_tickbuf(nullptr)
        {}

        // MsgSubSpi
        virtual void on_topic(const Msg& cont) override;

        void Init();

        void Start();
       
        void LoadAllMkt();

        void LoadOneMkt(MktId mktid);

        void LoadFile(const char* file);       

        void SaveAllMkt();

        void SaveOneMkt(MktId mktid);

        void SaveTick(list<QuoteData>& data);

        void TimerRun();

        bool Append(MarketDataInd& ind, bool no_log = false);
        bool AppendFromFile(MarketDataInd& ind, bool no_log = false);
        bool AppendQuote(MarketQuote* quote, bool no_log, bool from_file = false);
        
        bool GetData(const string& code, QuoteData* data);

        bool GetQuote(const string& code, MarketQuote* quote) {
            std::unique_lock<std::recursive_mutex> lck(m_mtx);
            int jzcode = jz_code(code.c_str());
            auto iter = m_data.find(jzcode);
            if (iter != m_data.end() && iter->second->initialized == true) {
                *quote = iter->second->last;
                if (quote->has_quoteage())
                    quote->set_quoteage(TimeUtil::dayMillis() - quote->quoteage());
                else
                    quote->set_quoteage(TimeUtil::dayMillis() - iter->second->last_arrived_time);
                return true;
            } else {
                return false;
            }
        }

    private:
        inline QuoteData* GetData(int jzcode) {
            auto iter = m_data.find(jzcode);
            return iter != m_data.end() ? iter->second : nullptr;
        }
        void MakeupBar();
        bool write_to_buffer(MarketDataInd& ind, const Msg& msg);        
        
    public:
        static QmsData* instance();
    private:
        map<uint32_t, QuoteData*> m_data;

        int m_trading_day;

        struct Buffer {
            int     capacity;
            char*   data;
            int     len;

            Buffer(int size) {
                data = new char[size];
                len = 0;
                capacity = size;
            }

            ~Buffer(){
                delete[] data;
            }

        };
        struct MktBufferData {
            list<Buffer*> m_tosaved;
            Buffer*       m_tickbuf;
            mutex         m_tosaved_mtx;
            bool          m_switched_to_night;
            uint32_t      m_trade_day;
            MktBufferData() : 
                    m_tickbuf(NULL), 
                    m_switched_to_night(false),
                    m_trade_day(0)
            {
            }
            ~MktBufferData()
            {
                for (auto it = m_tosaved.begin(); it != m_tosaved.end(); it++) {
                    delete (*it);
                }
                delete m_tickbuf;
            }
        };
        map<MktId, MktBufferData> m_buffer_map;
        list<Buffer*> m_tosaved;
        Buffer*       m_tickbuf;
        mutex         m_tosaved_mtx;
        bool          m_switched_to_night;
        msgbus::Subscriber*  m_mdlink;
        std::recursive_mutex m_mtx;
    };
};

#endif

