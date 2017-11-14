
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
#include <zmq.h>
#include <zlib.h>
#include <string>
#include <unistd.h>
#include "protocol/cpp/jzs.pb.h"
#include "protocol/cpp/qms.pb.h"
#include "protocol/cpp/md.pb.h"

struct MessageHead {
    uint32_t type;
    uint32_t body_length;
    char data[];
};

class QmsClient {
public:
    QmsClient() : m_zmqctx(nullptr), m_sock(nullptr)
    {
    }

    bool connect(const char* addr)
    {
        m_zmqctx = zmq_ctx_new();
        m_sock = zmq_socket(m_zmqctx, ZMQ_REQ);

        int rc = zmq_connect(m_sock, addr);
        if (rc!=0) {
            printf("Can't listen at %s, rc=%d\n", addr, rc);
            return false;
        }else
            return true;
    }

    template <class T>
    void send(int id, const T& req)
    {
        int len = req.ByteSize();
        zmq_msg_t msg;
        zmq_msg_init_size(&msg, len + sizeof(MessageHead));
        MessageHead* head = reinterpret_cast<MessageHead*>( zmq_msg_data(&msg));
        head->type = id;
        head->body_length = len;
        
        req.SerializeToArray(head->data, len);
        
        zmq_sendmsg(m_sock, &msg, 0);        
    }
    
    void expect(int id, jzs::msg::qms::MarketQuoteRsp& rsp)
    {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        int rc = zmq_recvmsg(m_sock, &msg, 0);
        
        assert (rc > 0);
         
        MessageHead* head = static_cast<MessageHead*>(zmq_msg_data(&msg));
        assert(head);
         
        assert (head->type==id);
        bool r = rsp.ParseFromArray(head->data, head->body_length);
        assert (r);

        zmq_msg_close(&msg);
    }

    jzs::msg::qms::MarketQuoteRsp getMarketData(const char* code)
    {
        char * s = new char[strlen(code)+1];
        strcpy(s, code);
       char* m = strrchr(s, '.');
        assert (m);
        *m = 0;
        m++;
        
        jzs::msg::qms::MarketQuoteReq req;
        //req.set_jzcode(1);
        req.set_symbol(code);
        int len = req.ByteSize();
        char* body = new char[len];
        req.SerializeToArray(body, len);
        
        send(jzs::msg::MSG_QMS_MARKETQUOTE_REQ, req);
        
        jzs::msg::qms::MarketQuoteRsp rsp;
        expect(jzs::msg::MSG_QMS_MARKETQUOTE_RSP, rsp);
        
        delete[] s;
        
        return rsp;
        
    }
    
private:
    void*  m_zmqctx;
    void*  m_sock;
    
};

std::string hhmmss_sss(int ms)
{
    char buf[100];
    int s = ms /1000;
    sprintf(buf, "%.2d:%.2d:%.2d.%.3d", (s/3600), (s/60)%60, s%60, ms %1000);
    return std::string(buf);
}

int main(int argc, const char** argv)
{
    const char* addr = "tcp://127.0.0.1:9000";
    if (argc==2)
        addr = argv[1];

    QmsClient c;
    if (!c.connect(addr))
        return -1;
 
    while (true ){
        auto rsp = c.getMarketData("IF1508.CFFEX");
        if (rsp.result() ){
            printf("%s %s %.2f %ld\n",
                rsp.symbol().c_str(),
                hhmmss_sss(rsp.data().time()).c_str(),
                rsp.data().last(), rsp.data().volume());
        }else {
            printf("failed!");
        }
        usleep(500000); // 0.5s
    }        
    
    return 0;
}
    
