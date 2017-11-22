
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
#include <mutex>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <sstream>
#include "base/Logger.h"
#include "base/TimeUtil.h"
#include "base/StringUtil.h"
#include "config/SysConfig.h"
#include "../public/MapTables.h"
#include "TdfMdServer.h"

using namespace std;
using namespace jzs;
using namespace jzs::msg;
using namespace jzs::msg::md;


static std::map<int, TdfMdServer*> g_serverMap;
static int g_nConnectionID = 0;
static std::mutex g_mutex;

TdfMdServer* getInstanceById(int id)
{
    std::unique_lock<std::mutex> lock(g_mutex);
    auto iter = g_serverMap.find(id);
    return (iter!=g_serverMap.end()?iter->second:nullptr);
}

void addInstance(int id, TdfMdServer* instance)
{
    std::unique_lock<std::mutex> lock(g_mutex);
    g_serverMap[id] = instance;
}

int getNextId()
{
    std::unique_lock<std::mutex> lock(g_mutex);
    return ++g_nConnectionID;
}


uint32_t TdfMdServer::find_jzsymbol_UPPER(const string& symbol)
{
    string code = symbol;
    transform(code.begin(), code.end(), code.begin(), ::toupper);    
    auto iter = m_maptables->m_uppersymbol_jzcode_map.find(code);
    if (iter != m_maptables->m_uppersymbol_jzcode_map.end())
        return iter->second;
    else
        return 0;
}

static std::map<string, string> g_tdcode_map;

static inline void init_map()
{
    static bool is_inited = false;
    if (is_inited) return;

    is_inited = true;
    g_tdcode_map["999999.SH"] = "000001.SH";
    g_tdcode_map["999998.SH"] = "000002.SH";
    g_tdcode_map["999997.SH"] = "000003.SH";
    g_tdcode_map["999996.SH"] = "000004.SH";
    g_tdcode_map["999995.SH"] = "000005.SH";
    g_tdcode_map["999994.SH"] = "000006.SH";
    g_tdcode_map["999993.SH"] = "000007.SH";
    g_tdcode_map["999992.SH"] = "000008.SH";
    g_tdcode_map["999991.SH"] = "000010.SH";
    g_tdcode_map["999990.SH"] = "000011.SH";
    g_tdcode_map["999989.SH"] = "000012.SH";
    g_tdcode_map["999988.SH"] = "000013.SH";
    g_tdcode_map["999987.SH"] = "000016.SH";
    g_tdcode_map["999986.SH"] = "000015.SH";
}

static string TDIndexCode_to_JZSymbol(const char* tdcode)
{
    init_map();

    auto iter = g_tdcode_map.find(tdcode);
    if (iter!=g_tdcode_map.end())
        return iter->second;

    if (tdcode[0]=='9' && tdcode[1]=='9' && strstr(tdcode, ".SH")) {
        char buf[100];
        snprintf(buf, 100, "00%s", tdcode+2);
        return string(buf);
    } else {
        return string(tdcode);
    }
}

TdfMdServer::TdfMdServer(): 
    m_reqid(0),
    m_thread(nullptr),
    m_shouldExit(false),
    m_tdf(nullptr),
    m_has_subscribed(false),
    m_today(0),
    m_data_count(0),
    m_subscribed_all(false)
{

}

bool TdfMdServer::init_by_type(MdlinkCfg& mdcfg)
{
    if (mdcfg.route != "TDF") {
        LOG(FATAL) << "Error md type " << mdcfg.route << " (TDF expected)";
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

TdfMdServer::~TdfMdServer()
{
    delete m_thread;
}


void TdfMdServer::Start()
{
    m_thread = new std::thread( &TdfMdServer::Run, std::ref(*this));
}

uint32_t TdfMdServer::tdcode_to_jzcode(const char* tdcode, const TDF_CODE_INFO * pCodeInfo, bool is_index)
{  
    string code(tdcode);
    switch (pCodeInfo->nMarketID) {
    case ID_MARKET_SZ:
        code += ".SZ";
        break;
    case ID_MARKET_SH:
        code += ".SH";
        break;
    case ID_MARKET_CF:
        code += ".CFE";
        break;
    case ID_MARKET_SHF:
        code += ".SHF";
        break;
    case ID_MARKET_CZC:
        code += ".CZC";
        break;
    case ID_MARKET_DCE:
        code += ".DCE";
        break;
    default:
        break;
    }
   
    auto it = m_subscribed_tdcode_map.find(code);
    if (it != m_subscribed_tdcode_map.end()) {
        return it->second;
    } else {
        //LOG_EVERY_N(WARNING, 100) << "No jzcode was found " << tdcode;

        if (!m_subscribed_all)
            return 0;

        string jzsymbol;
        if (is_index && pCodeInfo->nMarketID == ID_MARKET_SH)
            jzsymbol = TDIndexCode_to_JZSymbol(code.c_str());       
        else
            jzsymbol = code;

        // XXX 这里有线程同步问题，必须要保证 m_subscribed_tdcode_map
        // 只在回调函数中会被调用！
        uint32_t jzcode = find_jzsymbol_UPPER(jzsymbol);
        m_subscribed_tdcode_map[code] = jzcode;
        if (jzcode != 0) {          

            LOG(WARNING) << "Find new code " << tdcode << "->" << jzsymbol;

            auto pos = jzsymbol.rfind('.');
            if (pos != std::string::npos) {
                string mkt = jzsymbol.substr(pos + 1);
                string filename;
                int trading_date;
                if (get_ct_filename(mkt, &filename, &trading_date)){
                    ofstream out;
                    out.open(filename, ios::app | ios::out | ios::binary);
                    if (out.is_open()) {
                        if (!pCodeInfo)
                            out << trading_date
                            << "," << jzsymbol
                            << "," << tdcode
                            << "," << tdcode
                            << "," << tdcode
                            << "," << tdcode
                            << "," << 0
                            << ",,,,,,,,,,," << endl;
                        else {
                            out << trading_date
                                << "," << jzsymbol
                                << "," << tdcode
                                << "," << pCodeInfo->chCode
                                << "," << pCodeInfo->chName
                                << "," << pCodeInfo->chName
                                << "," << pCodeInfo->nType;

                            if (pCodeInfo->nType >= 0x90 && pCodeInfo->nType <= 0x95)
                                out << "," << pCodeInfo->exCodeInfo.Option.chContractID
                                << "," << pCodeInfo->exCodeInfo.Option.szUnderlyingSecurityID
                                << "," << pCodeInfo->exCodeInfo.Option.chCallOrPut
                                << "," << pCodeInfo->exCodeInfo.Option.nExerciseDate
                                << "," << pCodeInfo->exCodeInfo.Option.chOptionType
                                << "," << pCodeInfo->exCodeInfo.Option.chPriceLimitType
                                << "," << pCodeInfo->exCodeInfo.Option.nContractMultiplierUnit
                                << "," << pCodeInfo->exCodeInfo.Option.nExercisePrice
                                << "," << pCodeInfo->exCodeInfo.Option.nStartDate
                                << "," << pCodeInfo->exCodeInfo.Option.nEndDate
                                << "," << pCodeInfo->exCodeInfo.Option.nExpireDate << endl;
                            else
                                out << ",,,,,,,,,,," << endl;
                        }
                    }
                    out.close();
                }
            }            
        }
        return jzcode;
    }
}

void TdfMdServer::subscribe(THANDLE hTdf, TDF_CODE_RESULT* codeResult)
{
    bool is_ok = true;
    int today = TimeUtil::date();
    for (int i = 0; i < codeResult->nMarkets; i++){
        if (codeResult->nCodeDate[i] < today) {
            LOG(ERROR) << "Market has wrong date "
                << codeResult->szMarket[i] << " " << codeResult->nCodeDate[i];
            // TODO: 
            //is_ok = false;
            break;
        }
    }
    if (codeResult->nMarkets == 0){
        LOG(ERROR) << "No market";
        is_ok = false;
    }

    if (!is_ok) {
        TDF_Close(m_tdf);
        m_tdf = nullptr;
        return;
    }

    // 生成接收到的TD代码到jzcode的映射表
    // 注意：TD 代码表和JZ代码表区别
    //      1. CF vs CFE
    //      2. 指数名称不一致
    //      3. WindCode 全大小,  jzcode 市场大写，代码和交易所一致

    // 000001.SH -> 999999.SH
    // jzcode 转成大写
    std::map<string, string> jz_to_td;

    for (int i = 0; i < codeResult->nMarkets; i++){
        TDF_CODE * pCodes = NULL;
        uint32_t nItems =0 ;
        if (TDF_GetCodeTable(hTdf, codeResult->szMarket[i], &pCodes, &nItems)) {
    //    if (TDF_GetCodeTable(hTdf, "CFE", &pCodes, &nItems)) {
            LOG(ERROR) << "TDF_GetCodeTable FAILED " << codeResult->szMarket[i];
            continue;
        }
        bool is_cfe = (strcmp(codeResult->szMarket[i], "CF") == 0 || 
            strcmp(codeResult->szMarket[i], "CFE") ==0 || 
            strcmp(codeResult->szMarket[i], "CF-2-0") == 0);
        bool is_sh = (strcmp(codeResult->szMarket[i], "SH") == 0 ||
            strcmp(codeResult->szMarket[i], "SH-2-0") == 0
            );
        bool is_sz = (strcmp(codeResult->szMarket[i], "SZ") == 0 ||
            strcmp(codeResult->szMarket[i], "SZ-2-0") == 0
            );

        int trading_date = codeResult->nCodeDate[i];

        //// ct_tdf_CFE_20150819.csv
        string filename = build_ct_filename((is_cfe ? "CFE" : codeResult->szMarket[i]), trading_date);
        ofstream out;
        out.open(filename, ios::trunc | ios::out | ios::binary);
        if (out.is_open()) {
            LOG(INFO) << "Save code table " << filename;
            out << "date,jzcode,windcode,code,cn_name,en_name,type,ContractID,UnderlyingSecurityID,CallOrPut,ExerciseDate,OptionType,PriceLimitType,ContractMultiplierUnit,ExercisePrice,StartDate,EndDate,ExpireDate\n";
        }
        else
            LOG(ERROR) << "Save code table failed. Can't open " << filename;

        for (int n = 0; n < nItems; n++) {
            string jzsymbol;

            TDF_OPTION_CODE OptCode;
            bool isOption = (pCodes[n].nType >= 0x90 && pCodes[n].nType <= 0x95);
            bool isIndex = ((pCodes[n].nType & 0xF0) == 0x00);

            if (isIndex && is_sh) {
                string idx_code(pCodes[n].szCode);
                idx_code += ".SH";
                jzsymbol = TDIndexCode_to_JZSymbol(idx_code.c_str());
            }
            else
                jzsymbol = string(pCodes[n].szCode) + "." + (is_cfe ? "CFE" : string(pCodes[n].szMarket));

            if (isOption){
                if (TDF_GetOptionCodeInfo(hTdf, jzsymbol.c_str(), &OptCode) || (OptCode.nExpireDate <= 0 && OptCode.nExerciseDate <= 0)){
                    LOG(ERROR) << "TDF_GetOptionCodeInfo FAILED " << codeResult->szMarket[i] << " " << pCodes[n].szWindCode;
                    continue;
                }
            }

            if (out.is_open()){
                // 输出原始的jzcode
                out << trading_date
                    << "," << jzsymbol
                    << "," << pCodes[n].szWindCode
                    << "," << pCodes[n].szCode
                    << "," << pCodes[n].szCNName << "," << pCodes[n].szCNName
                    << "," << pCodes[n].nType;

                if (!isOption)
                    out << ",,,,,,,,,,," << endl;
                else
                    out << "," << OptCode.szContractID
                    << "," << OptCode.szUnderlyingSecurityID
                    << "," << OptCode.chCallOrPut
                    << "," << OptCode.nExerciseDate
                    << "," << OptCode.chOptionType
                    << "," << OptCode.chPriceLimitType
                    << "," << OptCode.nContractMultiplierUnit
                    << "," << OptCode.nExercisePrice
                    << "," << OptCode.nStartDate
                    << "," << OptCode.nEndDate
                    << "," << OptCode.nExpireDate << endl;
            }

            transform(jzsymbol.begin(), jzsymbol.end(), jzsymbol.begin(), ::toupper);
            string mkt = "";
            if (is_cfe) {
                mkt = "CFE";
            }
            else if (is_sh) {
                mkt = "SH";
            }
            else if (is_sz){
                mkt = "SZ";
            }
            jz_to_td[jzsymbol] = (string(pCodes[n].szCode) + "." + mkt);

        }

        out.close();

        if (pCodes) TDF_FreeArr(pCodes);
    }

    // 把订阅列表转换成 TDF的 subscription 字符串
    // 如果订阅了全市场，则不发送订阅字符串

    m_subscribed_tdcode_map.clear();

    if (!m_subscribed_all) {
        CHECK(m_subscribed_jzcodes.size() != 0);
        stringstream ss;
        int count = 0;
        for (auto& e : m_subscribed_jzcodes){
            auto iter = jz_to_td.find(e);
            if (iter != jz_to_td.end()){
                uint32_t jzcode = find_jzsymbol_UPPER(e);
                if (!jzcode) {
                    LOG(ERROR) << "No JZCODE was found " << e;
                    continue;
                }
                ss << iter->second << ";";
                m_subscribed_tdcode_map[iter->second] = jzcode;
                count++;

            } else {
                LOG(ERROR) << "Can't find jzcode " << e << " in TD code tables";
            }
        }
        LOG(INFO) << "TDF_SetSubscription " << count << " sybmols ";

    } else {

        CHECK(m_subscribed_jzcodes.size() == 0);

        // TDFAPI 存在一个问题：代码表不全，行情中会有增加的行情，因此全市场订阅时
        // 把全部的Jzcode 转换成 windcode
        //
        // 这里把TD代码表中代码先加入到MAP,后续新增加的在动态添加，参见 tdcode_to_jzcode
        for (auto & e : jz_to_td) {
            int jzcode = find_jzsymbol_UPPER(e.first);
            if (jzcode != 0) {
                m_subscribed_tdcode_map[e.second] = jzcode;
            }
        }

        LOG(INFO) << "Subscribe all markets. Total " << m_subscribed_tdcode_map.size() << " symbols";
    }

    m_has_subscribed = true;
}

void TdfMdServer::Run()
{
    LoadSymbol();

    m_subscribed_all = this->m_subscribed_jzcodes.size() == 0;

    int id = getNextId();
    addInstance(id, this);

    m_today = TimeUtil::date();

    int count = m_data_count;
    while (!m_shouldExit) {
        if (m_tdf) {
            if (chrono::seconds(60) < (chrono::system_clock::now() - m_heartbeat_time)){
#ifdef __linux__
                //DON'T CHANGE LOG FORMAT
                LOG(ERROR) << "Lost HEATBEAT, Reconnect";
                TDF_Close(m_tdf);
                m_tdf = nullptr;
#else
                LOG(ERROR) << "Lost HEATBEAT, don't reconnect on WINDOWS!!";
                m_heartbeat_time = chrono::system_clock::now();
#endif
            } else {
                sleep(2);
                m_today = TimeUtil::date();
                // 收到新数据包，也算心跳
                if (count != m_data_count){
                    count = m_data_count;
                    m_heartbeat_time = chrono::system_clock::now();
                }
            }
            continue;
        }

        // Connect
        if (m_cfg.addr.size()==0 || m_cfg.port <=0 ||
            m_cfg.username.size()==0 || m_cfg.passwd.size()==0 ||
            m_cfg.markets.size()==0 ) {
            printf("TDF configuration is wrong!\naddr=%s\nport=%d\nusername=%s\npasswd=%s\nmarkets=%s\n",
                   m_cfg.addr.c_str(),
                   m_cfg.port,
                   m_cfg.username.c_str(),
                   m_cfg.passwd.c_str(),
                   m_cfg.markets.c_str() );

            exit(-1);
        }

        TDF_OPEN_SETTING setting;

        memset(&setting, 0, sizeof(setting));
        setting.nConnectionID = id;
        strcpy(setting.szIp,    m_cfg.addr.c_str() );//"114.80.154.34"
        sprintf(setting.szPort, "%d", m_cfg.port); //6231
        strcpy(setting.szUser,  m_cfg.username.c_str() ) ;//
        strcpy(setting.szPwd,   m_cfg.passwd.c_str() ); //
        setting.pfnMsgHandler = RecvData;
        setting.pfnSysMsgNotify = RecvSys;
        setting.szMarkets = m_cfg.markets.c_str(); //"SZ;SH;CF";
        setting.szSubScriptions = "";// "000001.SZ";

        setting.nTime = 0;
		setting.nTypeFlags = 0; //(MSG_DATA_TRANSACTION | MSG_DATA_ORDERQUEUE | MSG_DATA_ORDER);

        TDF_ERR nErr = TDF_ERR_SUCCESS;
        THANDLE hTDF = TDF_Open(&setting, &nErr);

#ifdef __linux__
        TDF_SetEnv(TDF_ENVIRON_HEART_BEAT_INTERVAL, 5);
        TDF_SetEnv(TDF_ENVIRON_HEART_BEAT_FLAG, 2);
#endif
        if (!hTDF){
            LOG(ERROR) << "TDF_Open failed, error code=" << nErr;
            sleep(2);
            continue;
        } else{
            m_heartbeat_time = chrono::system_clock::now();
            m_tdf = hTDF;
        }
    }

    if (m_tdf!=NULL) {
        TDF_Close(m_tdf);
        m_tdf = NULL;
    }
}

void TdfMdServer::LoadSymbol()
{
    if (m_cfg.filter.empty()){
        LOG(WARNING) << "Empty filter list. Receive all data";
        return;
    }

    string symbol_file = SysConfig::getHomeDir() + "/" + string(m_cfg.filter.c_str());
    
    FILE *fp = fopen(symbol_file.c_str(), "r+");
    if (!fp) {
        LOG(ERROR) << "can't load filter symbols " << symbol_file;
        return;
    }

    char line[400];
    while (fgets(line, 400, fp) != NULL) {
        string str = trim(line);

        if(str[0]==';' || str[0]=='#' || str.size()==0) continue;

        if (find_jzsymbol_UPPER(str) == 0) {
            //string index_symbol = TDIndexCodeToWindCode(str.c_str());

            //if (find_jzcode(index_symbol.c_str()) == 0) {
            LOG(ERROR) << "can't find jzcode " << str;
            continue;
        }
        m_subscribed_jzcodes.insert(str);
    }

    LOG(INFO) << "TDF subscribe " << m_subscribed_jzcodes.size() << " symbols";

    fclose(fp);
}

//bool TdfMdServer::IsWanted(const char* windCode)
//{
//    return (m_subscribed_jzcodes.find(windCode)!=m_subscribed_jzcodes.end());
//}
//

void TdfMdServer::RecvData(THANDLE hTdf, TDF_MSG* pMsgHead)
{
    if (!pMsgHead) return;

    auto pThis = getInstanceById( pMsgHead->nConnectId);
    if (pThis)
        pThis->OnMarketData(pMsgHead);
   
}


static uint32_t hhMMSSmmmToTime(uint32_t t)
{
    uint32_t ms = t % 1000;
    uint32_t hms = t/1000;
    uint32_t s = hms % 100 + ((hms/100)%100)*60 + (hms/10000)*3600;

    return s*1000 + ms;
}

bool TdfMdServer::TdToJZS(const TDF_MARKET_DATA* md, MarketQuote* pbk)
{
    MarketQuote& bk = *pbk;
    uint32_t jzcode = tdcode_to_jzcode(md->szCode, md->pCodeInfo);
    if (!jzcode)
        return false;

    bk.set_jzcode(jzcode);

    auto qs = bk.mutable_qs();
    qs->set_tradeday    ( md->nTradingDay );
    qs->set_date        ( md->nActionDay );
    qs->set_downlimit   ( md->nLowLimited / 10000.0 );
    qs->set_uplimit     ( md->nHighLimited / 10000.0 );
    qs->set_preclose    ( md->nPreClose / 10000.0 );
    qs->set_presettle   ( 0 );
    qs->set_preinterest ( 0 );
    qs->set_predelta    ( 0 );

    auto ba = bk.mutable_ab();
    for ( int i = 0; i <5; i++) {
        ba->add_askvolume(md->nAskVol[i]);
        ba->add_bidvolume(md->nBidVol[i]);
        ba->add_askprice(md->nAskPrice[i] / 10000.0);
        ba->add_bidprice(md->nBidPrice[i] / 10000.0);
    }

#ifdef ENABLE_DEPTH_10
    for (int i = 5; i < 10; i++) {
        if (!md->nAskPrice[i] && !md->nBidPrice[i]) break;
        ba->add_askvolume(md->nAskVol[i]);
        ba->add_bidvolume(md->nBidVol[i]);
        ba->add_askprice(md->nAskPrice[i] / 10000.0);
        ba->add_bidprice(md->nBidPrice[i] / 10000.0);
    }
#endif
    bk.set_time     ( hhMMSSmmmToTime(md->nTime));
    bk.set_open     ( md->nOpen / 10000.0 );
    bk.set_high     ( md->nHigh / 10000.0 );
    bk.set_low      ( md->nLow / 10000.0 );
    bk.set_last     ( md->nMatch / 10000.0 );
    bk.set_volume   ( md->iVolume );
    bk.set_turnover ( md->iTurnover );
    bk.set_close    ( 0 );
    bk.set_settle   ( 0 );
    bk.set_interest ( 0 );
    bk.set_delta    ( 0 );    
    //bk.set_nbid         = 0 );
    //bk.set_nask         = 0 );
    //bk.set_ntrade       = 0 );
    bk.set_iopv      ( md->nIOPV/10000.0 );
    bk.set_avgaskpx  ( 0 );
    bk.set_avgbidpx  ( 0 );
    bk.set_totbidvol ( 0 );
    bk.set_totaskvol ( 0 );
    set_vwap(bk);
    return true;
}

bool TdfMdServer::TdToJZS(const TDF_FUTURE_DATA* md, MarketQuote* pbk)
{
    MarketQuote& bk = *pbk;
    uint32_t jzcode = tdcode_to_jzcode(md->szCode, md->pCodeInfo);
    if (!jzcode)
        return false;

    bk.set_jzcode(jzcode);

    auto qs = bk.mutable_qs();
    qs->set_tradeday(md->nTradingDay);
    qs->set_date(md->nActionDay);
    qs->set_downlimit(md->nLowLimited / 10000.0);
    qs->set_uplimit(md->nHighLimited / 10000.0);
    qs->set_preclose(md->nPreClose / 10000.0);
    qs->set_presettle(md->nPreSettlePrice / 10000.0);
    qs->set_preinterest(md->iPreOpenInterest);
    qs->set_predelta(md->nPreDelta / 10000.0);

    auto ba = bk.mutable_ab();
    for (int i = 0; i <5; i++) {
        ba->add_askvolume(md->nAskVol[i]);
        ba->add_bidvolume(md->nBidVol[i]);
        ba->add_askprice(md->nAskPrice[i] / 10000.0);
        ba->add_bidprice(md->nBidPrice[i] / 10000.0);
    }
#ifdef ENABLE_DEPTH_10
    for (int i = 5; i < 10; i++){
        ba->add_askvolume(0);
        ba->add_bidvolume(0);
        ba->add_askprice(0.0);
        ba->add_bidprice(0.0);
    }
#endif
    bk.set_time(hhMMSSmmmToTime(md->nTime));
    bk.set_open(md->nOpen / 10000.0);
    bk.set_high(md->nHigh / 10000.0);
    bk.set_low(md->nLow / 10000.0);
    bk.set_last(md->nMatch / 10000.0);
    bk.set_volume(md->iVolume);
    bk.set_turnover(md->iTurnover);
    bk.set_close(0);
    bk.set_settle(md->nSettlePrice / 10000.0);
    bk.set_interest(md->iOpenInterest);
    bk.set_delta(md->nCurrDelta / 10000.0);

    //bk.set_nbid         = 0 );
    //bk.set_nask         = 0 );
    //bk.set_ntrade       = 0 );
    bk.set_iopv(0);
    bk.set_avgaskpx(0);
    bk.set_avgbidpx(0);
    bk.set_totbidvol(0);
    bk.set_totaskvol(0);
    set_vwap(bk);
    return true;
}

bool TdfMdServer::TdToJZS(const TDF_INDEX_DATA* md, MarketQuote* pbk)
{
    uint32_t jzcode = tdcode_to_jzcode(md->szCode, md->pCodeInfo, true);
    if (!jzcode)
        return false;

    MarketQuote& bk = *pbk;

    bk.set_jzcode(jzcode);

    auto qs = bk.mutable_qs();
    qs->set_tradeday(md->nTradingDay);
    qs->set_date(md->nActionDay);
    qs->set_downlimit(0);
    qs->set_uplimit(0);
    qs->set_preclose(md->nPreCloseIndex / 10000.0);
    qs->set_presettle(0);
    qs->set_preinterest(0);
    qs->set_predelta(0);

    // FIXME: MarketQuote.ab is required
    // Should have IndexData for index
    auto ba = bk.mutable_ab();
    ba->add_askvolume(md->iTotalVolume);
    ba->add_bidvolume(md->iTotalVolume);
    ba->add_askprice(md->nLastIndex / 10000.0);
    ba->add_bidprice(md->nLastIndex / 10000.0);

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

    bk.set_time(hhMMSSmmmToTime(md->nTime));
    bk.set_open(md->nOpenIndex / 10000.0);
    bk.set_high(md->nHighIndex / 10000.0);
    bk.set_low(md->nLowIndex / 10000.0);
    bk.set_last(md->nLastIndex / 10000.0);
    bk.set_volume(md->iTotalVolume);
    bk.set_turnover(md->iTurnover);
    bk.set_close(0);
    bk.set_settle(0);
    bk.set_interest(0);
    bk.set_delta(0);

    //bk.set_nbid         = 0 );
    //bk.set_nask         = 0 );
    //bk.set_ntrade       = 0 );
    bk.set_iopv(0);
    bk.set_avgaskpx(0);
    bk.set_avgbidpx(0);
    bk.set_totbidvol(0);
    bk.set_totaskvol(0);
    set_vwap(bk);
    return true;
}

void TdfMdServer::OnMarketData(const TDF_MSG* pMsgHead)
{
    m_data_count++;    
    if (!m_has_subscribed)
        return;

    if (!pMsgHead->pData) return;
    unsigned int nItemCount = pMsgHead->pAppHead->nItemCount;
    unsigned int nItemSize = pMsgHead->pAppHead->nItemSize;
    if (!nItemCount) return;
  
    switch(pMsgHead->nDataType) {
    case MSG_DATA_MARKET: {
        const TDF_MARKET_DATA* p = static_cast<const TDF_MARKET_DATA*>(pMsgHead->pData);
        for (int i = 0; i < nItemCount; i++, p++) {
            MarketQuote bk;
            if (p->nTradingDay < m_today) {
                LOG_EVERY_N(ERROR, 1000) << "Drop yesteday data " << p->nTradingDay;
                break;
            }
            if (TdToJZS(p, &bk))
                Publish(MarketDataType::MD_STK_L1, bk);
        }
    }
    break;

    case MSG_DATA_FUTURE: 
        break;

    case MSG_DATA_INDEX: 
        {
            const TDF_INDEX_DATA* p = static_cast<const TDF_INDEX_DATA*>(pMsgHead->pData);
            for (int i = 0; i < nItemCount; i++, p++) {
                if (p->nTradingDay < m_today) {
                    LOG_EVERY_N(ERROR, 200) << "Drop yesteday data " << p->nTradingDay;
                    break;
                }

                MarketQuote bk;
                if (TdToJZS(p, &bk))
                    Publish(MarketDataType::MD_STK_L1, bk);
            }
        }
        break;
    case MSG_DATA_TRANSACTION:
        break;    
    case MSG_DATA_ORDERQUEUE:
        break;
    case MSG_DATA_ORDER:
        break;
    }
}

void TdfMdServer::Publish(jzs::msg::md::MarketDataType type, const MarketQuote& bk)
{
    publish(type, bk);
}


void TdfMdServer::RecvSys(THANDLE hTdf, TDF_MSG* pSysMsg)
{
    if (!pSysMsg ||! hTdf) {
        return;
    }

    switch (pSysMsg->nDataType) {
    case MSG_SYS_DISCONNECT_NETWORK: {
        LOG(ERROR)<<"MSG_SYS_DISCONNECT_NETWORK";
        TdfMdServer* pThis = getInstanceById(pSysMsg->nConnectId);
        if (pThis)
            pThis->m_has_subscribed = false;

    }
    break;
    case MSG_SYS_CONNECT_RESULT: {
        TDF_CONNECT_RESULT* pConnResult = (TDF_CONNECT_RESULT*)pSysMsg->pData;
        if (pConnResult && pConnResult->nConnResult) {
            LOG(INFO) << "Connected " << pConnResult->szIp << ":" << pConnResult->szPort
                << " " << pConnResult->szUser;
        } else {
            LOG(ERROR) << "Failed to connect "<< pConnResult->szIp << ":" << pConnResult->szPort
                << " " << pConnResult->szUser;
        }
    }
    break;
    case MSG_SYS_LOGIN_RESULT: {
        TDF_LOGIN_RESULT* pLoginResult = (TDF_LOGIN_RESULT*)pSysMsg->pData;
        if (pLoginResult && pLoginResult->nLoginResult) {
            LOG(INFO) << "Login sucess " << pLoginResult->szInfo << " nMarkets=" << pLoginResult->nMarkets;
            for (int i=0; i<pLoginResult->nMarkets; i++) {
                LOG(INFO) << pLoginResult->szMarket[i] << " " << pLoginResult->nDynDate[i];
            }
        } else {
            LOG(ERROR) << "Login failed: " << pLoginResult->szInfo;
        }
    }

    break;
    case MSG_SYS_CODETABLE_RESULT: {
        TDF_CODE_RESULT* pCodeResult = (TDF_CODE_RESULT*)pSysMsg->pData;
        if (pCodeResult ) {
            TdfMdServer* pThis = getInstanceById(pSysMsg->nConnectId);
            CHECK(pThis);

            for (int i=0; i<pCodeResult->nMarkets; i++) {
                LOG(INFO) << "CodeTable " << pCodeResult->szMarket[i]
                    << " total " << pCodeResult->nCodeCount[i]
                    << " " << pCodeResult->nCodeDate[i];
//                         if (1)  {
//                             TDF_CODE* pCodeTable;
//                             unsigned int nItems;
//                             TDF_GetCodeTable(hTdf, pCodeResult->szMarket[i], &pCodeTable, &nItems);
//                             printf("nItems =%d\n",nItems);
//                             for (int i=0; i < nItems; i++) {
//                                 TDF_CODE& code = pCodeTable[i];
//                                 //printf("windcode:%s, code:%s, market:%s, name:%s, nType:0x%x\n",code.szWindCode, code.szCode, code.szMarket, code.szCNName, code.nType);
//                             }
//                             TDF_FreeArr(pCodeTable);
//                         }
            }

            pThis->subscribe(hTdf, pCodeResult);
        }
    }
    break;
    case MSG_SYS_QUOTATIONDATE_CHANGE: {
        TDF_QUOTATIONDATE_CHANGE* pChange = (TDF_QUOTATIONDATE_CHANGE*)pSysMsg->pData;
        if (pChange)
            LOG(WARNING) << "MSG_SYS_QUOTATIONDATE_CHANGE: "<< pChange->szMarket << " "
                        << pChange->nOldDate << "=>" << pChange->nNewDate;
    }
    break;
    case MSG_SYS_MARKET_CLOSE: {
        TDF_MARKET_CLOSE* pCloseInfo = (TDF_MARKET_CLOSE*)pSysMsg->pData;
        if (pCloseInfo)
            LOG(INFO) << "MSG_SYS_MARKET_CLOSE";
    }
    break;
    case MSG_SYS_HEART_BEAT: {
        LOG(INFO) << "MSG_SYS_HEART_BEAT";

        TdfMdServer* pThis = getInstanceById(pSysMsg->nConnectId);
        if (pThis)
            pThis->m_heartbeat_time = chrono::system_clock::now();

        break;
    }
    default:
        break;
    }
}

/* 根据市场得到当前使用的 codetable文件和日期 */
bool TdfMdServer::get_ct_filename(const string& mkt, string* filename, int* tradedate)
{
    auto it = m_market_infos.find(mkt);
    if (it == m_market_infos.end())
        return false;

    if (filename) *filename = it->second.ct_filename;
    if (tradedate) *tradedate = it->second.trade_date;

    return true;
}

/* 生成 codetable 文件，并保存在 m_market_info 中供以后使用 */
string TdfMdServer::build_ct_filename(const string& mkt, int trading_date)
{
    stringstream ss;
    ss << SysConfig::getDataDir() << "/ct/ct_tdf_" << mkt << "_" << trading_date << ".csv";
    string filename = ss.str();
    TdfMarketInfo info;
    info.trade_date = trading_date;
    info.ct_filename = filename;

    m_market_infos[mkt] = info;
    return filename;
}


