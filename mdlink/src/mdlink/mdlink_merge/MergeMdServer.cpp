
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
#include <iostream>
#include <vector>
#include <limits>
#include <cmath>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <zmq.hpp>
#include "base/Logger.h"
#include "base/TimeUtil.h"
#include "base/StringUtil.h"
#include "config/SysConfig.h"
#include "MergeMdServer.h"
#include "../public/MapTables.h"
#ifdef __linux__
#include "shared_ptr_atomic.h"
#endif


using namespace jzs::msgbus;

using namespace std;
using namespace jzs;
using namespace jzs::msg;
using namespace jzs::msg::md;



struct QuoteInfo {
    string last_source;
    shared_ptr<MarketQuote> last_quote;
    jzs::msg::md::MarketDataType type;
};

static std::queue<QuoteInfo> g_quote_queue;
static std::condition_variable g_cv;
static std::mutex g_mutex;

MdClient::MdClient(const MdCfg* cfg) :   
    m_thread(nullptr)
{
    m_cfg = *cfg;
}

MdClient::~MdClient()
{
    
}

void MdClient::forward_publish(const jzs::msg::Msg& msg)
{
    if (msg.head().tid() != MSG_MD_MARKETDATA_IND)
        return;
    jzs::msg::md::MarketQuote* quote = nullptr;
        
    MarketDataInd ind;
    if (!ind.ParseFromString(msg.body()))
        return;
    string src = msg.head().src();
    switch (ind.type()) {
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
        return;
    }
    ;
    auto q = (new MarketQuote(*quote));    
    QuoteInfo info;
    info.last_quote = shared_ptr<MarketQuote> (q);
    info.last_source = src;
    info.type = ind.type();
    {
        unique_lock <mutex > lock(g_mutex);
        g_quote_queue.push(info);
        g_cv.notify_all();
    }
}


void MdClient::Start()
{
    if (m_thread) {
        LOG(ERROR) << "Can't receive and merge quotes simultaneously" << endl;
    }
    else {
        m_thread = new std::thread(&MdClient::Run, std::ref(*this));
    }
}


void MdClient::on_topic(const Msg& msg)
{
    if (msg.head().tid() != MsgType::MSG_MD_MARKETDATA_IND)
        return;

    forward_publish(msg);
}

void MdClient::Run()
{
    string up_addr = m_cfg.addr;

    if (up_addr.size() == 0) {
        LOG(FATAL) << "FWD mkdlik addre is empty";
        exit(-1);
    }

    LOG(INFO) << "Use UP mdlink " << up_addr.c_str();

    m_up_mdlink = msgbus::Subscriber::create(up_addr.c_str(), this);

    msgbus::Suber suber;
    suber.tid = jzs::msg::MsgType::MSG_MD_MARKETDATA_IND;
    m_up_mdlink->subscribe(suber);
    m_up_mdlink->start();   
}

MergeMdServer::~MergeMdServer()
{    
}

bool MergeMdServer::init_by_type(MdlinkCfg& mdcfg)
{
    if (mdcfg.route != "MERGE") {
        LOG(FATAL) << "Error md type " << mdcfg.route << " (MERGE expected)";
        return false;
    }
    vector<MdCfg> &mdvec = mdcfg.mdvec;    
    for (int i = 0; i < mdvec.size(); ++i) {
        auto client = new MdClient(&mdvec[i]);        
        m_clients.push_back(client);
    }
    return true;
}
void MergeMdServer::Start()
{
    if (m_thread) {
        LOG(ERROR) << "Can't receive and merge quotes simultaneously" << endl;
    }
    else {
        m_exec = true;
        for (int i = 0; i < m_clients.size(); i++) {
            m_clients[i]->Start();
        }
        m_thread = new std::thread(&MergeMdServer::Run, std::ref(*this));
    }
}

void MergeMdServer::Run()
{
    QuoteInfo pop_info;
    while (m_exec) {
        {
            unique_lock < mutex > lock(g_mutex);            
            if (!g_quote_queue.empty()) {
                pop_info = g_quote_queue.front();
                g_quote_queue.pop();
            }
            else {     
                g_cv.wait(lock);
              //  this_thread::sleep_for(std::chrono::microseconds(10));
                continue;
            }
        }
        auto quote = pop_info.last_quote;
        string src = pop_info.last_source;		
        publish(pop_info.type, *quote);
    }
}

void MergeMdServer::publish(jzs::msg::md::MarketDataType type,
    const jzs::msg::md::MarketQuote& quote)
{
    // 保存时间戳最新的记录    
    auto it = m_maptables->g_jzcode_to_mktinfo.find(quote.jzcode());
    if (it != m_maptables->g_jzcode_to_mktinfo.end()) {
        auto info = it->second;
        if (!info->latest_quote ||
            info->latest_quote->qs().date() < quote.qs().date() ||
            (info->latest_quote->time() < quote.time() && info->latest_quote->qs().date() == quote.qs().date())) {

            bool ignore = false;
            auto symbol = m_maptables->g_jzcode_to_symbol.find(quote.jzcode());
            
            // 简单过滤掉错误行情（包括夜盘行情）, 在当前时间30分钟之后的
            if (!ignore && TimeUtil::compare_time(quote.time(),
                TimeUtil::time_add(TimeUtil::dayMillis(), 30 * TimeUtil::millis_per_minute)) >= 0)
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

    // 主力合约等代码映射, 一个代码可能映射到多个代码上
   
}