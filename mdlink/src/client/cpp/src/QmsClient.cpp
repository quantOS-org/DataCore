
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
#include <map>
#include <zmq.h>
#include <zlib.h>
#include <string>
#include "jzs/QmsClient.h"

namespace jzs {

using namespace std;
using namespace ::jzs::msg::qms;
using namespace ::jzs::msg::md;
    

struct MessageHead {
    uint32_t type;
    uint32_t body_length;
    char data[];
};

QmsClient::QmsClient() : m_zmqctx(nullptr), m_sock(nullptr)
{
    m_zmqctx = zmq_ctx_new();

}

QmsClient::~QmsClient() {
    if (m_sock) zmq_close(m_sock);
    if (m_zmqctx) zmq_ctx_shutdown(m_zmqctx);
}

bool QmsClient::connect(const string& addr)
{
    m_addr = addr;
    return reconnect();
}

bool QmsClient::reconnect()
{
    if (m_sock)
        zmq_close(m_sock);

    m_sock = zmq_socket(m_zmqctx, ZMQ_REQ);

    int rc = zmq_connect(m_sock, m_addr.c_str());
    if (rc != 0) {
        printf("Can't listen at %s, rc=%d\n", m_addr.c_str(), rc);
        return false;
    }

    int to = 2000;
    zmq_setsockopt(m_sock, ZMQ_RCVTIMEO, &to, sizeof(to));
    to = 2000;
    zmq_setsockopt(m_sock, ZMQ_SNDTIMEO, &to, sizeof(to));
    return true;
}

shared_ptr<jzs::msg::qms::MarketQuoteRsp> QmsClient::getMarketData(const string& code)
{
    jzs::msg::qms::MarketQuoteReq req;
    req.set_symbol(code);

    send(jzs::msg::MSG_QMS_MARKETQUOTE_REQ, req);

    auto rsp = new jzs::msg::qms::MarketQuoteRsp();

    if ( !expect(jzs::msg::MSG_QMS_MARKETQUOTE_RSP, *rsp)) {
        delete rsp;
        return nullptr;
    }

    return shared_ptr<jzs::msg::qms::MarketQuoteRsp>(rsp);
}

shared_ptr<jzs::msg::qms::StrategyMarketQuotesRsp> QmsClient::getStrategyQuotes(uint32_t strategy_id)
{
    jzs::msg::qms::StrategyMarketQuotesReq req;
    req.set_req_id(0);
    req.set_strategy_id(strategy_id);

    send(jzs::msg::MSG_QMS_STRATEGY_MARKETQUOTES_REQ, req);

    auto rsp = new jzs::msg::qms::StrategyMarketQuotesRsp();

    if (!expect(jzs::msg::MSG_QMS_STRATEGY_MARKETQUOTES_RSP, *rsp)) {
        delete rsp;
        return nullptr;
    }

    return shared_ptr<jzs::msg::qms::StrategyMarketQuotesRsp>(rsp);
}

std::shared_ptr<jzs::msg::qms::StrategySubscribeRsp> QmsClient::strategySubsribe(uint32_t strategy_id, std::vector<string>& symbols)
{
    jzs::msg::qms::StrategySubscribeReq req;
    req.set_req_id(0);
    req.set_strategy_id(strategy_id);

    for (auto& e : symbols)
        *req.add_symbols() = e;

    send(jzs::msg::MSG_QMS_STRATEGY_SUBSCRIBE_REQ, req);

    shared_ptr<jzs::msg::qms::StrategySubscribeRsp> rsp(new jzs::msg::qms::StrategySubscribeRsp());

    if (!expect(jzs::msg::MSG_QMS_STRATEGY_SUBSCRIBE_RSP, *rsp)) {
        return nullptr;
    }

    return rsp;
}


shared_ptr<jzs::msg::qms::Bar1MRsp> QmsClient::getBar1M(const string& code)
{
    jzs::msg::qms::Bar1MReq req;
    req.set_symbol(code);

    send(jzs::msg::MSG_QMS_BAR_1M_REQ, req);

    auto rsp = new jzs::msg::qms::Bar1MRsp();

    if ( !expect(jzs::msg::MSG_QMS_BAR_1M_RSP, *rsp)) {
        delete rsp;
        return nullptr;
    }

    return shared_ptr<jzs::msg::qms::Bar1MRsp>(rsp);
}

template <class T>
void QmsClient::send(int id, const T& req)
{
    int len = req.ByteSize();
    zmq_msg_t msg;
    zmq_msg_init_size(&msg, len + sizeof(MessageHead));
    MessageHead* head = reinterpret_cast<MessageHead*>(zmq_msg_data(&msg));
    head->type = id;
    head->body_length = len;

    req.SerializeToArray(head->data, len);

    zmq_sendmsg(m_sock, &msg, 0);
}

template <class T>
bool QmsClient::expect(int id, T& rsp)
{
    zmq_msg_t msg;
    zmq_msg_init(&msg);
    int rc = zmq_recvmsg(m_sock, &msg, 0);

    do {
        if (rc <= 0)    break;

        if (zmq_msg_size(&msg) == 0) {
            zmq_msg_close(&msg);
            break;
        }

        MessageHead* head = static_cast<MessageHead*>(zmq_msg_data(&msg));

        bool ret = false;
        if (head->type == id) {
            if (head->body_length & 0x80000000){
                uint32_t len = head->body_length & 0x80000000;
                uint32_t buf_len = 100 * 1000; // 100K 
                Bytef* buf = nullptr;
                uLong un_len = 0;
                for (int i = 0; i < 5; i++){
                    buf = new  Bytef[buf_len];
                    un_len = buf_len;
                    int r = uncompress(buf, &un_len, (Bytef*)head->data, len);
                    if (r == Z_OK)
                        break;
                    else if (r == Z_BUF_ERROR) {
                        delete buf;
                        buf = nullptr;
                        len *= 2;
                    }
                    else {
                        delete buf;
                        buf = nullptr;
                        break;
                    }
                }

                if (buf) {
                    ret = rsp.ParseFromArray(buf, un_len);
                    delete buf;
                }
            }
            else {
                ret = rsp.ParseFromArray(head->data, head->body_length);
            }
        }

        zmq_msg_close(&msg);

        if (!ret) break;
        
        return true;

    } while (false);

    reconnect();

    return false;
}

}


