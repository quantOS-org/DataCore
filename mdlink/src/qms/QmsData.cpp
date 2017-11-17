
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
#include <cstdint>
#include <unistd.h>
#include <string>
#include <list>
#include <map>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <memory>
#include "base/Logger.h"
#include <math.h>
#include <fstream>
#ifdef __linux__
#include <sys/time.h>
#endif

#include "base/TimeUtil.h"
#include "base/Logger.h"
#include "base/Calendar.h"
#include "msgbus/Subscriber.h"
#include "base/StringUtil.h"
#include "Qms.h"
#include "QmsData.h"
#include "QmsBar.h"

using namespace std;

namespace jzs{
    
    using namespace ::std;
    int now_ms()
    {
#ifdef _WIN32
        SYSTEMTIME  time;
        GetLocalTime(&time);
        return (time.wHour * 3600 + time.wMinute * 60 + time.wSecond) * 1000 + time.wMilliseconds;
#else
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        // FIXME: timezone issue
        //return (tv.tv_sec % (3600*24)) * 1000 + tv.tv_usec/1000;

        time_t now = time(NULL);
        struct tm tm;
        localtime_r(&now, &tm);
        return (tm.tm_hour*3600 + tm.tm_min * 60 + tm.tm_sec ) * 1000 + tv.tv_usec / 1000;
#endif
        
    }    

    QmsData* QmsData::instance()
    {
        static QmsData* g_instance;
        if (g_instance) return g_instance;
        return (g_instance = new QmsData());
    }

    static
    string get_tkpath(int trading_day)
    {
        char h5filename[100];
        sprintf(h5filename, "%d.tk", trading_day);
        string path = SysConfig::getDataDir()+ string("/tk/") + string(h5filename);
        return path;
    }
    static 
    string get_tkpath(MktId mkt_id, int trading_day)
    {
        char h5filename[100];
        string mkt = g_market_map[mkt_id]->code;
        sprintf(h5filename, "%d.tk", trading_day);
        string path = SysConfig::getDataDir() + string("/tk/") + mkt + string(h5filename);
        return path;
    }
    // TK FILE FORMAT 
    //  1. flag(FF FF)  type(2bytes)  length(4bytes) body(length)
    //  2. flag(FF FE)  length(2bytes)  body(length)  // market quote
    void QmsData::SaveAllMkt()
    {
        if (!SysConfig::getQmsCfg()->save_tk) {
            return;
        }
        for (auto it = m_buffer_map.begin(); it != m_buffer_map.end(); it++) {
            SaveOneMkt(it->first);
        }
    }
    void QmsData::SaveOneMkt(MktId mktid)
    {
        auto it = m_buffer_map.find(mktid);
        MktBufferData& bd = it->second;
       
        string mktname = g_markets[mktid-1];
       
        if (!bd.m_trade_day) {
            LOG(ERROR) << "No trade day found for mkt " << mktname << " id "<< mktid << endl;
            return;
        }        
        string filename = get_tkpath(mktid, m_trading_day);
        FILE* fp = fopen(filename.c_str(), "ab");
        if (!fp) {
            LOG(ERROR) << "Can't open tick file " << filename;
            return;
        }
        fseek(fp, 0, SEEK_END);
        size_t len = ftell(fp);
        // Attention: 
        if (len == 0) {
            misc::CodeTable table;
            table.set_date(m_trading_day);
            {   
                for (auto it = g_tables->g_symbol_map.begin(); it != g_tables->g_symbol_map.end(); it++){
                    auto it_mkt = g_tables->g_jzcode_map.find(it->second.jzcode);
                    if (it_mkt != g_tables->g_jzcode_map.end()) {
                        // save code table by mkt
                        if (it_mkt->second.mktid == mktid) {
                            auto c = table.add_codes();
                            c->set_symbol(it->first);
                            c->set_jzcode(it->second.jzcode);
                        }
                    }
                }
            }
            string body;
            table.SerializeToString(&body);
            uint8_t head[8] = { 0xFF, 0xFF, 0x00, 0x01, 0, 0, 0, 0 };
            head[4] = (body.size() >> 24) & 0xFF;
            head[5] = (body.size() >> 16) & 0xFF;
            head[6] = (body.size() >> 8) & 0xFF;
            head[7] = (body.size()) & 0xFF;
            fwrite(head, 1, 8, fp);
            fwrite(body.data(), 1, body.size(), fp);
        }

        unique_lock<mutex> lock(bd.m_tosaved_mtx);
        list<Buffer*> list;
        {
            list = bd.m_tosaved;
            bd.m_tosaved.clear();
            if (bd.m_tickbuf && bd.m_tickbuf->len) {
                list.push_back(bd.m_tickbuf);
                bd.m_tickbuf = nullptr;
            }
        }
        for (auto it = list.begin(); it != list.end(); it++) {
            fwrite((*it)->data, 1, (*it)->len, fp);
            delete (*it);
        }
        fclose(fp);
    }
    
    void QmsData::LoadAllMkt()
    {
        std::vector<std::thread*> works;
        m_trading_day = g_tables->g_calendar->GetTradeDay(now_ms());

        /* single thread load file */
        /*
        for (auto it = g_market_map.begin(); it != g_market_map.end(); ++it) {
            m_buffer_map[it->first].m_trade_day = m_trading_day;
            LoadOneMkt(it->first);
        }
        */
        
        for (auto it = g_market_map.begin(); it != g_market_map.end(); ++it) {
            m_buffer_map[it->first].m_trade_day = m_trading_day;
            LOG(INFO) << "Load Market " << it->second->code << " " << m_trading_day << endl;
            works.push_back(new std::thread(&QmsData::LoadOneMkt, this, it->first));                            
        }

        for (auto it = works.begin(); it != works.end(); ++it) {
            (*it)->join();
        }
        
        for (auto it = works.begin(); it != works.end(); ++it) {
            delete *it;
        }
        
    }
    
    void QmsData::LoadOneMkt(MktId mktid)
    {    
        string path = get_tkpath(mktid, m_trading_day);
        LoadFile(path.c_str());
    }

    void QmsData::LoadFile(const char* file)
    {
        FILE* fp = fopen(file, "rb");
        string path(file);
        if (!fp) {
            LOG(ERROR) << "Can't open tick file " << path;
            return;
        }

        LOG(INFO) << "Load tick from " << path;

        uint32_t small_time = 0xFFFFFFFF, small_date = 0xFFFFFFFF, last_time = 0, last_date = 0;
        uint32_t ticks = 0;

        do {
            // read code table and compare wih g_symbol_map
            {
                uint8_t head[8];
                int len = fread(head, 1, 8, fp);
                uint8_t ct_head[4] = { 0xFF, 0xFF, 0x00, 0x01 };
                if (len != 8 || memcmp(head, ct_head, 4) != 0){
                    LOG(ERROR) << "Read tick file head error. rename it";
                    break;
                }

                int body_len = (head[4] << 24) | (head[5] << 16) | (head[6] << 8) | head[7];
                if (body_len <= 0 || body_len > 1024 * 1024 * 100) {
                    LOG(ERROR) << "Read tick file head error. rename it";
                    break;
                }
                char* body = new char[body_len];
                len = fread(body, 1, body_len, fp);
                if (len != body_len) {
                    LOG(ERROR) << "Read tick file code table error. rename it";
                    break;
                }
                misc::CodeTable table;
                if (!table.ParseFromArray(body, body_len)){
                    LOG(ERROR) << "Read tick file code table error. rename it";
                    break;
                }
                bool code_is_ok = true;
                for (int i = 0; i < table.codes_size(); i++) {
                    auto& c = table.codes(i);
                    if (jz_symbol(c.jzcode()) != c.symbol()){
                        LOG(ERROR) << "Unmatched code table " << c.jzcode() << " " << c.symbol() << " " << jz_symbol(c.jzcode());
                        code_is_ok = false;
                        break;
                    }
                }
                delete[] body;

                if (!code_is_ok && SysConfig::getQmsCfg()->check_code) break;
            }

            {
                bool data_is_ok = true;
                //body = new char[256 * 256];
                int max_len = 1024 * 1024 * (32 + 1);
                uint8_t* buf = new uint8_t[max_len];
                int len = 0;
                int pos = 0;
                while (true){
                    if (pos > max_len / 2) {
                        memmove(buf, buf + pos, len - pos);
                        len -= pos;
                        pos = 0;
                    }

                    if (len - pos < 4)
                        len += fread(buf + len, 1, max_len - len, fp);
                    if (len - pos == 0)
                        break;
                    if (len - pos < 4){
                        data_is_ok = false;
                        break;
                    }
                    uint8_t* head = buf + pos;
                    uint32_t body_size = 0;
                    if (head[0] == 0xFF && head[1] == 0xFE) {
                        pos += 4;
                        body_size = head[2] * 256 + head[3];
                    }
                    else {
                        pos += 6;
                        body_size = (head[2] << 24) | (head[3] << 16) | (head[4] << 8) | head[5];
                    }
                    if (len - pos < body_size) {
                        len += fread(buf + len, 1, max_len - len, fp);
                        if (len - pos < body_size) {
                            LOG(ERROR) << "Wrong data at " << ftell(fp);
                            data_is_ok = false;
                            break;
                        }
                    }

                    MarketDataInd ind;
                    if (!ind.ParseFromArray(buf + pos, body_size)) {
                        LOG(ERROR) << "Parse data failed at " << ftell(fp);
                        data_is_ok = false;
                        break;
                    }

                    pos += body_size;

                    AppendFromFile(ind, true);

                    const MarketQuote* quote = nullptr;
                    if (ind.has_fut())
                        quote = &ind.fut();
                    else if (ind.has_stk())
                        quote = &ind.stk();

                    ticks++;                    
                    
                    if (quote->time()) {
                        if (quote->qs().date() < small_date) {
                            small_date = quote->qs().date();
                            small_time = quote->time();
                            
                        }
                        else if (quote->time() < small_time && quote->qs().date() == small_date) {
                            small_time = quote->time();
                            
                        }
                        if (quote->qs().date() > last_date) {
                            last_date = quote->qs().date();
                            last_time = quote->time();
                        }
                        else if (quote->time() > last_time && quote->qs().date() == last_date) {
                            last_time = quote->time();                            
                        }
                    }

                }

                delete[] buf;

                if (data_is_ok) {
                    LOG(INFO) << "Load " << ticks << " ticks from " << small_date << " "
                        << TimeUtil::MillisToTime(small_time) << " to " << last_date << " " 
                        << TimeUtil::MillisToTime(last_time) << " " << path;
                    return;
                }
            }
        } while (false);

#ifndef _WIN32
        string bak = path + ".bak";
        rename(path.c_str(), bak.c_str());
        LOG(WARNING) << "Rename bad file " << path;
#endif
        return;
    }
   
    void QmsData::TimerRun()
    {
        static const int clear_time = TimeUtil::TimeToMillis(83000000);
        int timer_count = 0;
        // save header tables;
        SaveAllMkt();
        while (true) {
            sleep(1);
            timer_count++;   

            if (timer_count % 9 == 0) {                    
                SaveAllMkt();
            }    

            if (timer_count % 10 == 0) {
                LOG(INFO) << "I am alive!";
            }           
        }
    }

    void QmsData::Init()
    {
        m_trading_day = 0;
        m_switched_to_night = false;
        QuoteData* tmp_data = nullptr;
        for (auto it = g_tables->g_jzcode_map.begin(); it != g_tables->g_jzcode_map.end(); ++it) {
            // initialize m_data for tk file loading
            tmp_data = new QuoteData;
            tmp_data->mkt = g_market_map[it->second.mktid];
            m_data[it->first] = tmp_data;        
        }       
        
        chrono::time_point<chrono::system_clock> start = chrono::system_clock::now();
        LoadAllMkt();
        chrono::time_point<chrono::system_clock> end = chrono::system_clock::now();
        int dur = (int)chrono::duration_cast<std::chrono::milliseconds>(end - start).count();                  
        
        LOG(INFO) << "Read Time " << dur /1000 << endl;
    
        string mdlink_addr = SysConfig::getGlobalCfg()->mdlink_addr;
        if (mdlink_addr.size()==0) {
            LOG(FATAL) << "mdlink_addr is empty";
            exit(-1);
        }

        LOG(INFO) << "Use mdlink " << mdlink_addr.c_str();

        m_mdlink = msgbus::Subscriber::create(mdlink_addr.c_str(), this);

        msgbus::Suber suber;
        suber.tid = jzs::msg::MsgType::MSG_MD_MARKETDATA_IND;
        m_mdlink->subscribe(suber);
    }

    void QmsData::Start()
    {
        m_mdlink->start();     
        new std::thread(&jzs::QmsData::TimerRun, this);
    }

    bool QmsData::AppendFromFile(MarketDataInd& ind, bool no_log)
    {

        MarketQuote* quote = nullptr;
        switch (ind.type()){
        case MarketDataType::MD_FUT_L1:
        case MarketDataType::MD_FUT_L2:
            if (ind.has_fut())
                quote = ind.mutable_fut();
            break;
        case MarketDataType::MD_STK_L1:
        case MarketDataType::MD_STK_L2:
            if (ind.has_stk())
                quote = ind.mutable_stk();
            break;
        default:
            return false;
        }
        if (!quote) {
            LOG_EVERY_N(ERROR, 10) << "No quote data in MarketDataInd";
            return false;
        }
            
        return AppendQuote(quote, no_log, true);
    }

    bool QmsData::Append(MarketDataInd& ind, bool no_log)
    {
        MarketQuote* quote = nullptr;
        switch (ind.type()){
        case MarketDataType::MD_FUT_L1:
        case MarketDataType::MD_FUT_L2:
            if (ind.has_fut())
                quote = ind.mutable_fut();
            break;
        case MarketDataType::MD_STK_L1:
        case MarketDataType::MD_STK_L2:
            if (ind.has_stk())
                quote = ind.mutable_stk();
            break;
        default:
            return false;
        }
        if (!quote) {
            LOG_EVERY_N(ERROR, 10) << "No quote data in MarketDataInd";
            return false;
        }
        std::unique_lock<std::recursive_mutex> lck(m_mtx);
        bool res = AppendQuote(quote, no_log);
        return res;
    }

    // Attention: make sure it has a lock before calling this function;
    bool QmsData::AppendQuote(MarketQuote* quote, bool no_log, bool from_file) {        
        QuoteData* md = GetData(quote->jzcode());

        if (!md) {
            auto it = g_tables->g_jzcode_map.find(quote->jzcode());
            if (it  !=  g_tables->g_jzcode_map.end()) {
                md = new QuoteData;
                md->mkt = g_market_map[it->second.mktid];
                m_data[it->first] = md;
            }      
            else {
                LOG(ERROR) << "Quote Data of " << quote->jzcode() << " can't be found!";
                return false;
            }
        }    

        if (md->initialized == false ||
            md->last.qs().date() < quote->qs().date() ||
            (md->last.qs().date() == quote->qs().date() && md->last.time() <= quote->time())) {    
            md->last = *quote;
            if (md->initialized == false)
                // First tick received
                md->initialized = true;
        }
        else {
            LOG_EVERY_N(ERROR, 10) << "Tick time roll back: " << jz_symbol(quote->jzcode()) << " last time is " << md->last.qs().date() <<
                " " << TimeUtil::MillisTimeStrMs(md->last.time()) << ", tick time is " << quote->qs().date() 
                << " " << TimeUtil::MillisTimeStrMs(quote->time());
            return false;
        }
        // 保存接收时间到 quote中，这样当重新启动qms后能够还原接收时间。
        // 注意：
        //   1. quote_age 是当前时间和接收时间的差，这里借用了这个变量，在 GetQuote中转换
        //   2.  假设mdlink 发送的行情中没有quote_age，否则的话  mdlink和qms
        //     必须运行在同一台服务器上
        if (!quote->has_quoteage() && ! from_file) {        
            quote->set_quoteage(TimeUtil::dayMillis());
        }     

        if (from_file) {
            md->mkt->last_arrived_time = md->last_arrived_time = quote->time();
        }
        else {
            md->mkt->last_arrived_time = md->last_arrived_time = quote->quoteage();
        }

        // FIXE: support night market       
        if ( (md->mkt->last_time < quote->time() && md->mkt->last_date == quote->qs().date()) || 
            md->mkt->last_date < quote->qs().date()) {
            md->mkt->last_time = quote->time();
            md->mkt->last_date = quote->qs().date();
        }  

        if (quote->volume() != 0) {
            Bar1M_Insert(md);
        }

        return true;
    }

    bool QmsData::write_to_buffer(MarketDataInd& ind, const Msg& msg)
    {
        MarketQuote* quote = nullptr;
        switch (ind.type()){
        case MarketDataType::MD_FUT_L1:
        case MarketDataType::MD_FUT_L2:
            if (ind.has_fut())
                quote = ind.mutable_fut();
            break;
        case MarketDataType::MD_STK_L1:
        case MarketDataType::MD_STK_L2:
            if (ind.has_stk())
                quote = ind.mutable_stk();
            break;
        default:
            ;
        }
        if (quote && quote->has_jzcode()) {            
            auto it = g_tables->g_jzcode_map.find(quote->jzcode());
            if (it == g_tables->g_jzcode_map.end()) {
                LOG_EVERY_N(ERROR, 10) << "Unknown jzcode " << quote->jzcode();
                return false;
            }                    
            MktBufferData& bd = m_buffer_map[it->second.mktid];
            unique_lock<mutex> lock(bd.m_tosaved_mtx);
            Time time = TimeUtil::MillisToTime(quote->time());
            MarketInfo* mkt_info = g_market_map[it->second.mktid];       
                   
            
            if (!bd.m_tickbuf)
                bd.m_tickbuf = new Buffer(1024 * 1024 * 10);

            const string body = msg.body();

            if (bd.m_tickbuf->len + msg.body().size() + 4 > bd.m_tickbuf->capacity) {
                bd.m_tosaved.push_back(bd.m_tickbuf);
                bd.m_tickbuf = new Buffer(1024 * 1024 * 10);
            }

            uint8_t head[4] = { 0xFF, 0xFE, uint8_t(body.size() / 256), uint8_t(body.size() & 0xFF) };
            memcpy(bd.m_tickbuf->data + bd.m_tickbuf->len, head, 4);
            memcpy(bd.m_tickbuf->data + bd.m_tickbuf->len + 4, body.data(), body.size());
            bd.m_tickbuf->len += 4 + body.size();
            return true;
        }
        return false;
        
    }

    void QmsData::on_topic(const Msg& msg)
    {
        if (msg.head().tid() != MsgType::MSG_MD_MARKETDATA_IND)
            return;

        MarketDataInd ind;

        if (!ind.ParseFromString(msg.body()))
            return;
        
        if (Append(ind)){
            if (!write_to_buffer(ind, msg)) {
                return;
            }
        }
    }

    void QmsData::MakeupBar()
    {
        std::unique_lock<std::recursive_mutex> lck(m_mtx);

        for (auto it = m_data.begin(); it != m_data.end(); it++) {
            auto qd = it->second;
            if (qd->bar_1m.size() > 0 && 
                (qd->last_bardate < qd->mkt->last_bardate ||
                (qd->last_bartime) < qd->mkt->last_bartime && qd->last_bardate == qd->mkt->last_bardate)) {
                LOG(WARNING) << "makeup bar1m " << jz_symbol(it->first);
                Bar1M_MakeupTo(qd, qd->mkt->last_bartime);
            }
        }
    }
    
    bool QmsData::GetData(const string& code, QuoteData* data)
    {
        std::unique_lock<std::recursive_mutex> lck(m_mtx);
        int jzcode = jz_code(code.c_str());
        auto iter = m_data.find(jzcode);
        if (iter != m_data.end()) {
            QuoteData* qd = iter->second;
            if (qd->bar_1m.size() > 0 &&  
                (qd->last_bardate < qd->mkt->last_bardate || 
                qd->last_bartime < qd->mkt->last_bartime && qd->last_bardate == qd->mkt->last_bardate ) ) {
                LOG(WARNING) << "makeup bar1m " << code;
                Bar1M_MakeupTo(qd, qd->mkt->last_bartime);
            }

            *data = *iter->second;

            if (data->last.has_quoteage())
                data->last.set_quoteage(TimeUtil::dayMillis() - data->last.quoteage());
            else
                data->last.set_quoteage(TimeUtil::dayMillis() - iter->second->last_arrived_time);
            return true;

            return true;
        } else {
            return false;
        }
    }
}
