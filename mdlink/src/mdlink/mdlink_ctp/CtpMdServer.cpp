
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
#include <assert.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <cmath>
#include <cstring>
#include <thread>
#include "base/Logger.h"
#include "base/TimeUtil.h"
#include "base/StringUtil.h"
#include "config/SysConfig.h"
#include "CtpMdServer.h"
#include <unistd.h>
#include "../public/MapTables.h"

using namespace std;

using namespace jzs;
using namespace jzs::msg::md;

int CtpMdServer::ctp_jzcode(const char* mkt, const char* symbol)
{
    assert(m_maptables->g_ctpcode_map.size());
    auto iter = m_maptables->g_ctpcode_map.find(symbol);
    return iter != m_maptables->g_ctpcode_map.end() ? iter->second : 0;
}

string CtpMdServer::ctp_symbol(int jzcode)
{
    assert(m_maptables->g_ctpsymbol_map.size());
    auto iter = m_maptables->g_ctpsymbol_map.find(jzcode);
    return iter != m_maptables->g_ctpsymbol_map.end() ? iter->second : "";
}

CtpMdServer::CtpMdServer()
{
    reqid = 0;
    userapi = NULL;   
}

CtpMdServer::~CtpMdServer()
{
    ;
}


//void CtpMdServer::Register(MdClient *client){
//  this->client = client;
//}

void CtpMdServer::Start()
{
    new thread(&CtpMdServer::Run, this);
}

bool CtpMdServer::init_by_type(MdlinkCfg& mdcfg)
{
    if (mdcfg.route != "CTP") {
        LOG(FATAL) << "Error md type " << mdcfg.route << " (CTP expected)";
        return false;
    }
    vector<MdCfg> &mdvec = mdcfg.mdvec;

    if (mdvec.size() != 1) {
        LOG(FATAL) << "More than one ctp sources are given";
        return false;
    }

    m_cfg = mdvec[0];
    return true;
}


void CtpMdServer::Run()
{
    LoadSymbol();

    string flow = SysConfig::getDataDir() + "/flow/md_" + m_cfg.id + "_";
    LOG(INFO) << "Use flow: " << flow;

    userapi = CThostFtdcMdApi::CreateFtdcMdApi(flow.c_str(), m_cfg.udp, m_cfg.multicast);
    userapi->RegisterSpi(this);

    for (int i = 0; i < m_cfg.front.size(); ++i) {
        userapi->RegisterFront(const_cast<char*>(m_cfg.front[i].c_str()));
        LOG(INFO) << "Use CTP front: " << m_cfg.front[i];
     }
    userapi->Init();    
    userapi->Join();
    LOG(FATAL) << "CTP exit!";
}


void CtpMdServer::LoadSymbol()
{
    m_subscribed.clear();

    if (m_cfg.filter.size()) {
        string file = SysConfig::getHomeDir() + "/" + m_cfg.filter;

        FILE *fp = fopen(file.c_str(), "r+t");
        if (fp) {
            LOG(INFO) << "Load symbols from " << file;

            char instline[400];
            while (fgets(instline, 400, fp) != NULL) {
                if (instline[0] == ';' || instline[0] == '#') continue;
                char *exg = strtok(instline, ", \t\r\n"); // exchange
                char *sym = strtok(NULL, ", \t\r\n"); // symbol
                if (exg == NULL || *exg == 0 || sym == NULL || *sym == 0)
                    continue;
                m_subscribed.push_back(sym);
            }
            LOG(INFO) << "Total symbols " << m_subscribed.size();

            if (!m_subscribed.size())
                LOG(WARNING) << "No symbol was loaded. Load from database";
        } else  {
            LOG(ERROR) << "Can't open symbol file " << file;
        }
    }

    if (m_subscribed.size()) return;

    for (auto& e : m_maptables->g_ctpcode_map)
        m_subscribed.push_back(e.first);

    LOG(INFO) << "Load " << m_subscribed.size() << " symbols from database";
}

void CtpMdServer::OnRspError(CThostFtdcRspInfoField *pRspInfo,
                             int nRequestID, bool bIsLast)
{
    IsErrorRspInfo(pRspInfo);
}

void CtpMdServer::OnFrontDisconnected(int nReason)
{
    LOG(ERROR) << "Disconect from ctp front." << " Reason code "<<nReason ;
}

void CtpMdServer::OnHeartBeatWarning(int nTimeLapse)
{
    LOG(WARNING) << "Receive ctp Heartbeat timeout.";
}

void CtpMdServer::OnFrontConnected()
{
    LOG(INFO) << "Connect to ctp sucessfully.";
    LOG(INFO) << "Send login (" << m_cfg.broker << "," << m_cfg.investor << "," << m_cfg.passwd << ")";

    CThostFtdcReqUserLoginField req;
    memset(&req, 0, sizeof(req));
    strcpy(req.BrokerID, m_cfg.broker.c_str());
    strcpy(req.UserID,   m_cfg.investor.c_str());
    strcpy(req.Password, m_cfg.passwd.c_str());
    int ret = userapi->ReqUserLogin(&req, ++reqid);
    
    // FIXME: should reconnect later instead of ASSERT
    CHECK(ret == 0) << "Send ReqUserLogin Failed.";
}

void CtpMdServer::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                 CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (!IsErrorRspInfo(pRspInfo) && pRspUserLogin) {
        LOG(INFO) << "Login to ctp success. trading day=" << pRspUserLogin->TradingDay;
    }
    string time(TimeUtil::timestr());
    string dbg_log = SysConfig::getDataDir() + "/log/ctpmd_" + m_cfg.id + "_"+ time + "_sub.txt";
    LOG(INFO) << "Save subscribed list to " << dbg_log;
    fstream out;
    out.open(dbg_log, ios::trunc | ios::out);

    char **insts = new char*[m_subscribed.size()];
    for (size_t i = 0; i < m_subscribed.size(); ++i) {
        insts[i] = (char*)m_subscribed[i].c_str();
        if (out.is_open())
            out << insts[i] << endl;
    }

    if (out.is_open()) out.close();

    int ret = userapi->SubscribeMarketData(insts, m_subscribed.size());
    LOG(INFO) << m_subscribed.size() << " symbols subscribed. ";
    CHECK(ret == 0) << "SubscribeMarketData Failed.";
    delete[] insts;
}

void CtpMdServer::resubscribe()
{
    m_subscribed.clear();
    for (auto& e : m_maptables->g_ctpcode_map)
        m_subscribed.push_back(e.first);
    char **insts = new char*[m_subscribed.size()];
    for (size_t i = 0; i < m_subscribed.size(); ++i) {
        insts[i] = (char*)m_subscribed[i].c_str();
    }
    int ret = userapi->SubscribeMarketData(insts, m_subscribed.size());
    LOG(INFO) << m_subscribed.size() << " symbols subscribed. ";
    CHECK(ret == 0) << "SubscribeMarketData Failed.";
    delete[] insts;
}

void CtpMdServer::OnRspSubMarketData(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

void CtpMdServer::OnRspUnSubMarketData(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
}

inline double CtpMdServer::Norm(double price)
{
    static double eps = 0.0000001f;
    static double dbl_max = (numeric_limits<double>::max)();

    if (fabs(price - dbl_max) < eps)
        return 0;
    else return price;
}

void CtpMdServer::OnRtnDepthMarketData(
    CThostFtdcDepthMarketDataField *pd)
{
    MarketQuote bk;
    int jzcode = ctp_jzcode(pd->ExchangeID, pd->InstrumentID);
    if (!jzcode)
        return;

    bk.set_jzcode(jzcode);

    auto qs = bk.mutable_qs();
    qs->set_tradeday(atoi(pd->TradingDay));
    qs->set_date(atoi(pd->ActionDay));
    qs->set_downlimit(Norm(pd->LowerLimitPrice));
    qs->set_uplimit(Norm(pd->UpperLimitPrice));
    qs->set_preclose(Norm(pd->PreClosePrice));
    qs->set_presettle(Norm(pd->PreSettlementPrice));
    qs->set_preinterest(Norm(pd->PreOpenInterest));
    qs->set_predelta(Norm(pd->PreDelta));

    auto ba = bk.mutable_ab();

    ba->add_askvolume(pd->AskVolume1);
    ba->add_bidvolume(pd->BidVolume1);
    ba->add_askprice(Norm(pd->AskPrice1));
    ba->add_bidprice(Norm(pd->BidPrice1));
#ifdef ENABLE_DEPTH_10
    int depth = 10;
#else
    int depth = 5;
#endif
    for (int i = 1; i < depth; i++){
        ba->add_askvolume(0);
        ba->add_bidvolume(0);
        ba->add_askprice(0.0);
        ba->add_bidprice(0.0);
    }

    bk.set_time(TimeUtil::hhmmss(pd->UpdateTime, pd->UpdateMillisec));
    bk.set_open(Norm(pd->OpenPrice));
    bk.set_high(Norm(pd->HighestPrice));
    bk.set_low(Norm(pd->LowestPrice));
    bk.set_last(Norm(pd->LastPrice));
    bk.set_volume(pd->Volume);
    Instrument inst = m_maptables->get_instrument(jzcode);
    if (inst.init) {
        double turnover = Norm(pd->Turnover);
        if (inst.mkt == 4) {
            turnover *= inst.multiplier;
        }
        bk.set_turnover(turnover);
    }
    else {
        LOG(ERROR) << "Can't find jzcode " << jzcode << endl;
        return;
    }
    bk.set_close(Norm(pd->ClosePrice));
    bk.set_settle(Norm(pd->SettlementPrice));
    bk.set_interest(Norm(pd->OpenInterest));
    bk.set_delta(Norm(pd->CurrDelta));

    bk.set_iopv(0);
    bk.set_avgaskpx(0);
    bk.set_avgbidpx(0);
    bk.set_totbidvol(0);
    bk.set_totaskvol(0);
    set_vwap(bk);

    publish(MarketDataType::MD_FUT_L1, bk);
}

inline bool CtpMdServer::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    bool err = ((pRspInfo) && (pRspInfo->ErrorID != 0));
    if (err) {
        LOG(ERROR)<< pRspInfo->ErrorMsg;
    }
    return err;
}
