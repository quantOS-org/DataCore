
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
#include <string>
#include <list>
#include <map>
#include <stdio.h>
#include <zmq.h>
#include <thread>
#include <chrono>
#include <zlib.h>
#include <queue>
#include <unistd.h>
#include <iostream>
#include <random>
#include <atomic>
#ifdef __linux__
#include "shared_ptr_atomic.h"
#endif

#include "base/Logger.h"
#include "base/TimeUtil.h"
#include "base/StringUtil.h"
#include "config/IniApi.h"
#include "QmsData.h"
#include "QmsSvr.h"
#include "Qms.h"
#include "protocol/cpp/jzs.h"

namespace jzs{

    using namespace std;
    using namespace chrono;

    struct MessageHead {
        uint32_t type;
        uint32_t body_length;
    };

    string bin2hex(const char* p, int size)
    {
        string str; str.resize(size * 3 + 1);
        char* dst = (char*)str.c_str();

        for (int i = 0; i < size; i++, dst += 3, p++)
            sprintf(dst, "%.2X ", (uint8_t)*p);

        *dst = '\0';
        return str;
    }

    string bin2hex(const zmq::message_t * msg)
    {
        return bin2hex((const char*)msg->data(), msg->size());
    }

    int random(int min_v, int max_v) {
        typedef std::minstd_rand G;
        static G g(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count());
        typedef std::uniform_int_distribution<> D;
        D d(min_v, max_v);
        return d(g);
    }

    const char* backend_addr = "inproc://qmssvr.backend";

    void simclient_run(int port);

    QmsSvr::QmsSvr() :
        m_ctx(nullptr),
        m_backend(nullptr),
        m_frontend(nullptr)
    {

    }

    void QmsSvr::init()
    {
        m_addr = SysConfig::getQmsCfg()->addr;
        m_id = SysConfig::getQmsCfg()->id;

        if (m_addr.size() == 0) {
            printf("qms addr is empty!\n");
            exit(-1);
        }
        if (m_id.size() == 0) {
            printf("qms id is empty!\n");
            exit(-1);
        }

        m_ctx = new zmq::context_t();

        // inproc: need in same context
        for (int i = 1; i <= 10; i++)
            new QmsSvr_Worker(m_ctx, i);


        auto ss = split(m_addr, ":");
        if (ss.size() == 1) {
            LOG(FATAL) << "wrong addr format, no port!" << m_addr;
        }

        int port = atoi(ss[ss.size() - 1].c_str());
        if (port <= 0) {
            LOG(FATAL) << "wrong port, " << port;
        }

        new thread(simclient_run, port);
    }

    struct WorkerInfo {
        zmq::message_t addr_msg;
        system_clock::time_point last_time;
    };

    void QmsSvr::send_heartbeat_req(WorkerInfo* worker)
    {
        //LOG(INFO) << "Send heartbeat req to worker " << bin2hex(&worker->addr_msg);

        zmq::message_t req_msg(sizeof(MessageHead));
        MessageHead* head = static_cast<MessageHead*>(req_msg.data());
        head->type = MSG_LOW_HEARTBEAT_REQ;
        head->body_length = 0;

        zmq::message_t empty_msg;
        m_backend->send(worker->addr_msg, ZMQ_SNDMORE); // worker_addr
        m_backend->send(empty_msg, ZMQ_SNDMORE);
        m_backend->send(empty_msg, ZMQ_SNDMORE);    //client_addr
        m_backend->send(empty_msg, ZMQ_SNDMORE);
        m_backend->send(req_msg);                   // data
    }

    shared_ptr<WorkerInfo> QmsSvr::forward_reply()
    {
        //LOG(INFO) << "forward_reply 1";

        auto worker = make_shared<WorkerInfo>();
        worker->last_time = system_clock::now();

        zmq::message_t empty_msg;
        zmq::message_t client_addr_msg;
        zmq::message_t reply_msg;

        if (!m_backend->recv(&worker->addr_msg, ZMQ_DONTWAIT) || !worker->addr_msg.more()){
            LOG(ERROR) << "Wrong message order 1";
            return nullptr;
        }
        if (!m_backend->recv(&empty_msg, ZMQ_DONTWAIT) || !empty_msg.more()) {
            LOG(ERROR) << "Wrong message order 2";
            return nullptr;
        }
        if (!m_backend->recv(&client_addr_msg, ZMQ_DONTWAIT) || !client_addr_msg.more()) {
            LOG(ERROR) << "Wrong message order 3";
            return nullptr;
        }

        if (!m_backend->recv(&empty_msg, ZMQ_DONTWAIT) || !empty_msg.more()) {
            LOG(ERROR) << "Wrong message order 4";
            return nullptr;
        }
        if (!m_backend->recv(&reply_msg, ZMQ_DONTWAIT))  {
            LOG(ERROR) << "Wrong message order 5";
            return nullptr;
        }

        //LOG(INFO) << "forward_reply 2 worker_addr " << bin2hex(&worker->addr_msg);
        //LOG(INFO) << "forward_reply 2 client_addr " << bin2hex(&client_addr_msg);

        MessageHead* head = reinterpret_cast<MessageHead*>(reply_msg.data());
        if (head->type != MSG_LOW_HEARTBEAT_RSP) {
            m_frontend->send(client_addr_msg, ZMQ_SNDMORE);
            m_frontend->send(empty_msg, ZMQ_SNDMORE);
            m_frontend->send(reply_msg);
        }
        else {
            //LOG(INFO) << "Recv heartbeat rsp from worker " << bin2hex(&worker->addr_msg);
        }

        const uint8_t* p = (const uint8_t*)worker->addr_msg.data();
        if (!p || *p > 0x0A || worker->addr_msg.size() != 4) {
            LOG(ERROR) << "Wrong worker addr " << bin2hex(&worker->addr_msg);
        }
        //LOG(INFO) << "forward_reply 3";

        return worker;
    }

    void QmsSvr::forward_request(WorkerInfo* worker)
    {
        //LOG(INFO) << "forward_request 1";
        zmq::message_t client_addr_msg;
        zmq::message_t empty_msg;
        zmq::message_t request_msg;

        if (!m_frontend->recv(&client_addr_msg, ZMQ_DONTWAIT) || !client_addr_msg.more()) {
            LOG(ERROR) << "Wrong message order";
            return;
        }
        if (!m_frontend->recv(&empty_msg, ZMQ_DONTWAIT) || !empty_msg.more()) {
            LOG(ERROR) << "Wrong message order";
            return;
        }
        if (!m_frontend->recv(&request_msg, ZMQ_DONTWAIT)) {
            LOG(ERROR) << "Wrong message order";
            return;
        }

        if (request_msg.more()) {
            LOG(ERROR) << "Wrong message order";
            while (m_frontend->recv(&request_msg, ZMQ_DONTWAIT) && request_msg.more()) {}
            return;
        }

        m_backend->send(worker->addr_msg, ZMQ_SNDMORE);
        m_backend->send(empty_msg, ZMQ_SNDMORE);
        m_backend->send(client_addr_msg, ZMQ_SNDMORE);
        m_backend->send(empty_msg, ZMQ_SNDMORE);
        m_backend->send(request_msg);

        //LOG(INFO) << "forward_request 2";
    }

    void QmsSvr::check_status()
    {
        //LOG(INFO) << "check_status";

        auto now = system_clock::now();
        if (now - m_last_worker_time > seconds(4)) {
            LOG(ERROR) << "lost worker, close backend socket";
            if (m_backend) {
                delete m_backend;
                m_backend = nullptr;
            }
        }

        if (now - m_last_client_time > seconds(4)) {
            LOG(ERROR) << "lost client, close frontend socket";
            if (m_frontend) {
                delete m_frontend;
                m_frontend = nullptr;
            }
        }

        if (!m_backend) {
            LOG(INFO) << "Create backend socket on " << backend_addr;
            m_backend = new zmq::socket_t(*m_ctx, ZMQ_ROUTER);
            m_backend->bind(backend_addr);
            m_last_worker_time = now;
        }

        if (!m_frontend) {
            LOG(INFO) << "Create frontend socket on " << m_addr << "," << m_id;
            m_frontend = new zmq::socket_t(*m_ctx, ZMQ_ROUTER);
            m_frontend->setsockopt(ZMQ_IDENTITY, m_id.c_str(), m_id.size()); // Does it need?
            m_frontend->bind(m_addr.c_str());
            m_last_client_time = now;
        }

        // send heartbeat every 1 seconds
        for (auto it = m_worker_queue.begin(); it != m_worker_queue.end();){
            if (now - (*it)->last_time > seconds(1)){

                //LOG_EVERY_N(INFO, 1) << "Send heartbeat to worker" << bin2hex( it->get()->addr_msg); 

                send_heartbeat_req(it->get());

                auto e = it;
                it++;
                m_worker_queue.erase(e);
            }
            else {
                //LOG(INFO) << "worker time is ok " << duration_cast<milliseconds>(now - (*it)->last_time).count() << " ms";
                it++;
            }
        }

    }

    void QmsSvr::forward_data()
    {
        zmq::pollitem_t items[] = {
            { *m_backend, 0, ZMQ_POLLIN, 0 },   //  Always poll for worker activity on m_backend
            { *m_frontend, 0, ZMQ_POLLIN, 0 }   //  Poll front-end only if we have available workers
        };

        //LOG(ERROR) << "worker count: " << m_worker_queue.size();

        if (m_worker_queue.size()) {
            int r = zmq::poll(&items[0], 2, 1000); // 1 SECONDS
            LOG_IF(ERROR, r < 0) << "zmq_poll frontend & backend return " << r;
            if (r <= 0) return;
        }
        else {
            // no worker, wait for any worker's response
            int r = zmq::poll(&items[0], 1, 1000); //
            LOG_IF(ERROR, r < 0) << "zmq_poll backend return " << r;

            if (r <= 0) return;
        }

        //  Handle worker activity on m_backend
        if (items[0].revents & ZMQ_POLLIN) {

            //  Queue worker address for LRU routing
            auto worker = forward_reply();

            if( worker ) {
	        m_worker_queue.push_back(worker);
            }

            m_last_worker_time = system_clock::now();
        }

        if (items[1].revents & ZMQ_POLLIN) {

            m_last_client_time = system_clock::now();
            auto worker = m_worker_queue.front();
            forward_request(worker.get());
            m_worker_queue.pop_front();
        }
    }


    void QmsSvr::run()
    {
        // addr message
        std::list<shared_ptr<WorkerInfo>> worker_queue;

        m_last_worker_time = system_clock::now();
        m_last_client_time = system_clock::now();

        while (true) {
            try {
                check_status();

                forward_data();

            }
            catch (std::exception& e){
                LOG(ERROR) << "zmq::proxy throws exception: " << e.what();
                std::this_thread::sleep_for(chrono::milliseconds(10));
            }
        }
    }

    // call getMarketQuoteReq every 1 seconds

    void simclient_run(int port)
    {
        stringstream ss; ss << "tcp://127.0.0.1:" << port;
        string addr = ss.str();

        zmq::context_t ctx;
        zmq::socket_t* sock = nullptr;

        auto last_time = system_clock::now();

        while (true) {
            try {
                if (!sock) {
                    LOG(INFO) << "Monitor connects to " << addr;
                    sock = new zmq::socket_t(ctx, ZMQ_REQ);
                    int to = 2000;
                    sock->setsockopt(ZMQ_RCVTIMEO, &to, sizeof(to));
                    sock->setsockopt(ZMQ_SNDTIMEO, &to, sizeof(to));
                    sock->connect(addr.c_str());
                }

                auto now = system_clock::now();
                if (now - last_time < seconds(2)) {
                    this_thread::sleep_for(milliseconds(500));
                    continue;
                }

                //LOG(INFO) << "send MarketQuoteReq to server";
                jzs::msg::qms::MarketQuoteReq req;
                req.set_symbol("abcdef.SH");
                int len = req.ByteSize();
                zmq::message_t msg(len + sizeof(MessageHead));
                MessageHead* head = reinterpret_cast<MessageHead*>(msg.data());
                head->type = jzs::msg::MSG_QMS_MARKETQUOTE_REQ;
                head->body_length = len;

                req.SerializeToArray(head + 1, len);

                zmq::message_t rsp_msg;
                if (sock->send(msg, 0) && sock->recv(&rsp_msg)) {
                    last_time = system_clock::now();
                    //LOG(INFO) << "recv MarketQuoteRsp from server";
                    continue;
                }
                LOG(ERROR) << "call MarketQuoteReq failed. close it";
                sock->close();
                delete sock;
                sock = nullptr;
            }
            catch (exception& e) {
                LOG(ERROR) << "monitor catches exepction: " << e.what();
                if (sock) {
                    delete sock;
                    sock = nullptr;
                }
                this_thread::sleep_for(milliseconds(500));
            }
        }
    }
    QmsSvr_Worker::QmsSvr_Worker(zmq::context_t* ctx, int id) :
        m_ctx(ctx),
        m_id(id),
        m_should_exit(false),
        m_sock(nullptr)
    {
        m_thread = new thread(&QmsSvr_Worker::run, this);
    }

    void QmsSvr_Worker::onMaketQuoteReq(const char* data, size_t size)
    {
        jzs::msg::qms::MarketQuoteReq req;
        if (!req.ParseFromArray(data, size)){
            LOG(ERROR) << "Parse MarketQuoteReq failed";
            jzs::msg::qms::MarketQuoteRsp rsp;
            rsp.set_symbol("");
            rsp.set_result(false);
            rsp.clear_data();
            sendRsp(jzs::msg::MSG_QMS_MARKETQUOTE_RSP, rsp);
            return;
        }

        LOG_EVERY_N(INFO, 1000) << "OnMaketQuoteReq " << req.symbol();

        jzs::msg::qms::MarketQuoteRsp rsp;
        rsp.set_symbol(req.symbol());

        MarketQuote* quote = rsp.mutable_data();
        if (QmsData::instance()->GetQuote(req.symbol(), quote)){
            rsp.set_result(true);
            if (!rsp.IsInitialized()) {
                LOG(ERROR) << "DATA is not ready " << req.symbol();
                rsp.clear_data();
                rsp.set_result(false);
            }
        }
        else {
            rsp.set_result(false);
            rsp.clear_data();
        }

        sendRsp(jzs::msg::MSG_QMS_MARKETQUOTE_RSP, rsp);
    }

    static void assign(qms::SimpleMarketQuote* to, const string& symbol, const QuoteData* from)
    {
        to->set_is_empty(false);
        to->set_symbol(symbol);
        to->set_time(from->last.time());
        to->set_open(from->last.open());
        to->set_high(from->last.high());
        to->set_low(from->last.low());
        to->set_last(from->last.last());
        to->set_volume(from->last.volume());
        to->set_turnover(from->last.turnover());
        to->set_interest(from->last.interest());
        to->set_vwap(from->last.vwap());
        *to->mutable_ab() = from->last.ab();
        *to->mutable_qs() = from->last.qs();
        to->set_iopv(from->last.iopv());
        to->set_quoteage(TimeUtil::dayMillis() - from->last_arrived_time);
    }

    static void assign_empty(qms::SimpleMarketQuote* to, const string& symbol)
    {
        to->set_is_empty(true);
        to->set_symbol(symbol);
        to->set_time(0);
        to->set_open(0);
        to->set_high(0);
        to->set_low(0);
        to->set_last(0);
        to->set_volume(0);
        to->set_turnover(0);
        to->set_interest(0);
        to->set_vwap(0);
        to->mutable_ab();
        to->mutable_qs()->set_date(0);
        to->mutable_qs()->set_tradeday(0);
        to->mutable_qs()->set_uplimit(0);
        to->mutable_qs()->set_downlimit(0);
        to->mutable_qs()->set_preinterest(0);
        to->mutable_qs()->set_preclose(0);
        to->mutable_qs()->set_presettle(0);
        to->mutable_qs()->set_predelta(0);

        to->set_iopv(0);

    }

    void QmsSvr_Worker::onStrategySubscribeReq(const char* data, size_t size)
    {
        jzs::msg::qms::StrategySubscribeReq req;
        if (!req.ParseFromArray(data, size)){
            LOG(ERROR) << "Can't parse StrategyMarketQuotesReq";
            return;
        }

        jzs::msg::qms::StrategySubscribeRsp rsp;
        rsp.set_req_id(req.req_id());
        rsp.set_strategy_id(req.strategy_id());

        auto it = g_universe_map.find(req.strategy_id());

        if (it != g_universe_map.end()) {

            rsp.set_result(true);

            shared_ptr<UniverseData> old_univ = atomic_load(&it->second);

            shared_ptr<UniverseData> new_univ(new UniverseData());
            for (auto& e : old_univ->codes){
                if (!e.subscribed)
                    new_univ->append(e.jzcode, e.symbol, false);
            }            
            for (int i = 0; i < req.symbols_size(); i++){
                const string& symbol = req.symbols(i);
                uint32_t jzcode = jz_code(symbol);
                if (jzcode){
                    new_univ->append(jzcode, symbol, true);             
                }
                else {
                    rsp.set_result(false);
                    rsp.add_err_symbols(symbol);
                }
            }
            atomic_store(&g_universe_map[req.strategy_id()], new_univ);
        }
        else {
            // TODO: 今后如果strategy不存在，也可以订阅，但需要注意
            // 在多线程QmsSvr下，如果是新增加策略，则有 map 内部指针调整
            //   导致正在访问这个Map的线程出错！
            //   可以使用 atomic或spinlock解决
            for (int i = 0; i < req.symbols_size(); i++){
                *rsp.mutable_err_symbols() = req.symbols();
            }
            rsp.set_result(false);
        }
        sendRsp(jzs::msg::MSG_QMS_STRATEGY_SUBSCRIBE_RSP, rsp);
    }

    bool time_compare(MillisTime t1, MillisTime t2)
    {
        if (TimeUtil::compare_time(t1, t2) < 0) {
            return true;
        }
        else {
            return false;
        }
    }
    void QmsSvr_Worker::onStrategyMarketQuotesReq(const char* data, size_t size)
    {
        jzs::msg::qms::StrategyMarketQuotesReq req;
        if (!req.ParseFromArray(data, size)){
            LOG(ERROR) << "Can't parse StrategyMarketQuotesReq";
            return;
        }

        LOG_EVERY_N(INFO, 100) << "OnStrategyMarketQuotesReq " << req.strategy_id();

        bool result = false;

        jzs::msg::qms::StrategyMarketQuotesRsp rsp;

        auto it = g_universe_map.find(req.strategy_id());
        if (it != g_universe_map.end()) {
            auto universe = atomic_load(&it->second);

            int now = TimeUtil::dayMillis();
            // 只等待两次
            for (int i = 0; i < 2; i++) {
                bool ok = true;
                for (auto& e : universe->markets){
                    if (TimeUtil::compare_time(TimeUtil::time_add(e.second->last_arrived_time, 1), now) > 0) {
                        ok = false;
                        usleep(2 * 1000);
                        break;
                    }
                }
                if (ok) break;
            }

            QmsData* qms_data = QmsData::instance();
            unique_lock<recursive_mutex> lock(qms_data->m_mtx);

            vector<MillisTime> arrive_time;

            for (auto& e : universe->codes) {
                auto iter = qms_data->m_data.find(e.jzcode);
                auto simple_quote = rsp.add_quotes();
                if (iter != qms_data->m_data.end() && iter->second->initialized == true) {
                    assign(simple_quote, e.symbol, iter->second);
                    arrive_time.push_back(iter->second->last_arrived_time);
                }
                else
                    assign_empty(simple_quote, e.symbol);
            }

            int middle_age = 0;
            if (arrive_time.size() > 0) {
                sort(arrive_time.begin(), arrive_time.end(), time_compare);
                middle_age = TimeUtil::time_sub(now, arrive_time[arrive_time.size() / 2]);
            }
            rsp.set_middle_age(middle_age);
            result = true;
        }

        rsp.set_req_id(req.req_id());
        rsp.set_result(result);
        rsp.set_strategy_id(req.strategy_id());

        if (!rsp.IsInitialized()) {
            LOG(ERROR) << "DATA is not ready " << req.strategy_id();
            rsp.clear_quotes();
            rsp.set_result(false);
        }

        sendRsp(jzs::msg::MSG_QMS_STRATEGY_MARKETQUOTES_RSP, rsp);
    }

    template <class T>
    void AssignTo(T* lhd, const vector<Bar>& rhd)
    {
        for (auto iter = rhd.begin(); iter != rhd.end(); iter++){
            auto r = iter;
            if (!r->is_auct) continue;
            auto l = lhd->Add();
            l->set_date(r->date);
            l->set_time(r->bar_time);
            l->set_open(r->open);
            l->set_high(r->high);
            l->set_low(r->low);
            l->set_close(r->close);
            l->set_vwap(r->vwap);
            l->set_match_item(r->match_item);
            l->set_volume(r->volume_total);
            l->set_turnover(r->turnover_total);
            l->set_interest(r->interest_total);
            l->set_volume_inc(r->volume_inc);
            l->set_turnover_inc(r->turnover_inc);
            l->set_interest_inc(r->interest_inc);
            l->set_flag(r->flag);
        }
    }

    void QmsSvr_Worker::onBar1MReq(const char* data, size_t size)
    {
        jzs::msg::qms::Bar1MReq req;
        jzs::msg::qms::Bar1MRsp rsp;

        if (req.ParseFromArray(data, size)){

            LOG_EVERY_N(INFO, 100) << "OnBar1MReq " << req.symbol();

            rsp.set_symbol(req.symbol());
            QuoteData d;
            if (QmsData::instance()->GetData(req.symbol(), &d)){
                rsp.set_result(true);
                jzs::msg::qms::Bar1M* rsp_bar1m = rsp.mutable_data();

                AssignTo(rsp_bar1m->mutable_bar(), d.bar_1m);
                *rsp_bar1m->mutable_last() = d.last;

                if (!rsp.IsInitialized()){
                    LOG(ERROR) << "DATA is not ready " << req.symbol();
                    rsp.clear_data();
                    rsp.set_result(false);
                }
            }
            else {
                rsp.set_result(false);
            }
        }
        else {
            LOG(ERROR) << "Parse Bar1MReq failed";
            rsp.set_symbol("");
            rsp.set_result(false);
        }

        sendRsp(jzs::msg::MSG_QMS_BAR_1M_RSP, rsp);
    }

    void QmsSvr_Worker::sendRsp(uint32_t type, const google::protobuf::Message& rsp)
    {
        //LOG(INFO) << "sendRsp " << m_id;

        //if (rsp.ByteSize() <= 1000*1000) {
        int size = sizeof(MessageHead) + rsp.ByteSize();
        zmq::message_t msg(size);

        char* p = static_cast<char*>(msg.data());

        MessageHead* head = reinterpret_cast<MessageHead*>(p);;
        head->type = type;
        head->body_length = rsp.ByteSize();

        rsp.SerializeToArray(p + sizeof(MessageHead), size - sizeof(MessageHead));

        try {
            m_sock->send(m_addr_msg, ZMQ_SNDMORE);
            m_sock->send(m_empty_msg, ZMQ_SNDMORE);
            m_sock->send(msg);
        }
        catch (...){
            LOG(ERROR) << "zmq: throw exception " << zmq_strerror(zmq_errno());
        }
    }

    void QmsSvr_Worker::onHeartBeatReq(const char* data, size_t size)
    {
        misc::LowHeartBeatRsp rsp;
        rsp.set_req_id(0);
        rsp.set_req_time(0);
        rsp.set_rsp_time(0);
        sendRsp(jzs::msg::MSG_LOW_HEARTBEAT_RSP, rsp);
    }

    void QmsSvr_Worker::run()
    {
        this_thread::sleep_for(milliseconds(200));
        LOG(INFO) << "Start worker " << m_id;

        auto last_time = system_clock::now();

        while (true) {
            try{
                if (system_clock::now() - last_time > seconds(3) && m_sock) {
                    LOG(ERROR) << "recv timeout 3 seconds, reconnect socket";
                    m_sock->close();
                    delete m_sock;
                    m_sock = nullptr;
                }
                if (m_sock == nullptr) {
                    LOG(INFO) << "Worker " << m_id << " connect to " << backend_addr;
                    m_sock = new zmq::socket_t(*m_ctx, ZMQ_REQ);
                    int to = 2000;
                    m_sock->setsockopt(ZMQ_RCVTIMEO, &to, sizeof(to));
                    m_sock->setsockopt(ZMQ_SNDTIMEO, &to, sizeof(to));

                    int id = m_id + random(1, 65535) * 256;
                    m_sock->setsockopt(ZMQ_IDENTITY, &id, sizeof(id));

                    m_sock->connect(backend_addr);

                    // notify QmsSvr the new worker
                    m_addr_msg.rebuild(6); memcpy(m_addr_msg.data(), "SERVER", 6);
                    onHeartBeatReq(nullptr, 0);

                    last_time = system_clock::now();;
                }

                if (!m_sock->recv(&m_addr_msg) || !m_addr_msg.more()){
                    continue;
                }
                if (!m_sock->recv(&m_empty_msg) || !m_empty_msg.more() ||
                    !m_sock->recv(&m_request_msg)){
                    LOG(ERROR) << "message order is not correct!";
                }

                last_time = system_clock::now();;

            }
            catch (exception& e) {
                LOG_EVERY_N(ERROR, 10) << "sock->recv throw exception: " << m_id << ", " << e.what();
                this_thread::sleep_for(milliseconds(100));
                if (m_sock) {
                    delete m_sock;
                    m_sock = nullptr;
                }
                continue;
            }


            const int HEAD_SIZE = sizeof(MessageHead);

            const char* p = static_cast<const char*>(m_request_msg.data());
            size_t size = m_request_msg.size();
            const MessageHead* head = reinterpret_cast<const MessageHead*>(p);

            // Wrong packet, reply with the request
            if (head->body_length + HEAD_SIZE != size) {
                m_sock->send(m_addr_msg, ZMQ_SNDMORE);
                m_sock->send(m_empty_msg, ZMQ_SNDMORE);
                m_sock->send(m_request_msg);
                continue;
            }

            switch (head->type){
            case jzs::msg::MSG_LOW_HEARTBEAT_REQ:
                onHeartBeatReq(p + HEAD_SIZE, head->body_length);

                break;
            case jzs::msg::MSG_QMS_MARKETQUOTE_REQ:
                onMaketQuoteReq(p + HEAD_SIZE, head->body_length);
                break;

            case jzs::msg::MSG_QMS_BAR_1M_REQ:
                onBar1MReq(p + HEAD_SIZE, head->body_length);
                break;

            case jzs::msg::MSG_QMS_STRATEGY_MARKETQUOTES_REQ:
                onStrategyMarketQuotesReq(p + HEAD_SIZE, head->body_length);
                break;

            case jzs::msg::MSG_QMS_STRATEGY_SUBSCRIBE_REQ:
                onStrategySubscribeReq(p + HEAD_SIZE, head->body_length);
                break;

            default:
                LOG_EVERY_N(ERROR, 100) << "Unknown request" << head->type;
                // TODO: repsonse with UNKNOW_REQUEST message

                m_sock->send(m_addr_msg, ZMQ_SNDMORE);
                m_sock->send(m_empty_msg, ZMQ_SNDMORE);
                m_sock->send(m_request_msg);

                break;
            }
        }
    }
}

