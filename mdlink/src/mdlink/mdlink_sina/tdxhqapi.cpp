#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <string.h>
#include <iostream>
#include "tdxhqapi.h"

#include "zlib.h"


#ifndef _WIN32

typedef int SOCKET;
#define INVALID_SOCKET -1
#define closesocket ::close

#else

typedef int socklen_t;
#include <Windows.h>
#include <winsock.h>
#include <WinSock2.h>
#endif

#include <vector>

using namespace std;
using namespace std::chrono;

using namespace tdxhqapi;

/*
return INADDR_NONE if failed
FIXME: may block system!
*/
static
uint32_t resolve_name(const string& name)
{
    hostent* h = gethostbyname(name.c_str());
    if (!h) return INADDR_NONE;

    if (h->h_addrtype != AF_INET) return 0;
    // return first addr
    return *(uint32_t*)h->h_addr;
}

TdxHqApi::TdxHqApi()
    : m_sock(0)
    , m_tradingday(0)
{

}

TdxHqApi::~TdxHqApi()
{
    if (m_sock) closesocket(m_sock);
}

bool TdxHqApi::create_connection(const string& ip, int port)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        addr.sin_addr.s_addr = resolve_name(ip);
        if (addr.sin_addr.s_addr == INADDR_NONE) {
            cerr << "Can't resolve name " << ip << endl;;
            return false;
        }
    }
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        cerr << "Can't create socket" << endl;;
        return false;
    }

    if (::connect(s, (sockaddr*)&addr, sizeof(addr)) == -1) {
        cerr << "connect error: " << ip << "," << strerror(errno) << endl;;
        closesocket(s);
        return false;
    }

    m_sock = s;
    m_ip = ip;
    m_port = port;
    return true;
}


bool TdxHqApi::connect(const string&ip, int port)
{
    return create_connection(ip, port) && login();
}

void TdxHqApi::init_codetable(const vector<Code>& codes)
{
    m_codetable.clear();
    for (auto & c : codes)
        m_codetable[c.code] = c;
}

bool TdxHqApi::login()
{
    static const uint8_t req_1[] = { 0x0c, 0x02, 0x18, 0x93, 0x00, 0x01, 0x03, 0x00, 0x03, 0x00, 0x0d, 0x00, 0x01 };
    static const uint8_t req_2[] = { 0x0c, 0x02, 0x18, 0x94, 0x00, 0x01, 0x03, 0x00, 0x03, 0x00, 0x0d, 0x00, 0x02 };
    static const uint8_t req_3[] = {
        0x0c, 0x03, 0x18, 0x99, 0x00, 0x01, 0x20, 0x00, 0x20, 0x00, 0xdb, 0x0f, 0xd5, 0xd0, 0xc9, 0xcc,
        0xd6, 0xa4, 0xc8, 0xaf, 0x00, 0x00, 0x00, 0x8f, 0xc2, 0x25, 0x40, 0x13, 0x00, 0x00, 0xd5, 0x00,
        0xc9, 0xcc, 0xbd, 0xf0, 0xd7, 0xea, 0x00, 0x00, 0x00, 0x02 };

    const uint8_t * req[3] = { req_1, req_2, req_3 };
    int   req_len[3] = { sizeof(req_1), sizeof(req_2), sizeof(req_3) };

    bool ret = false;

    for (int i = 0; i < 3; i++) {

        uint8_t* rsp = nullptr;
        int rsp_size = 0;
        if (!call(req[i], req_len[i], &rsp, &rsp_size))
            break;

        if (i == 0) {
            m_tradingday = *(int*)(rsp + 50);
            m_server_name = (char*)(rsp + 68);

            cerr << "server info: " << m_server_name << "," << m_tradingday << endl;;
        }

        delete[] rsp;

        if (i == 2) ret = true;
    }

    return ret;
}

static inline uint8_t get_byte(const uint8_t**p)
{
    const uint8_t r = *(uint8_t*)*p;
    (*p)++;
    return r;
}

static inline uint16_t get_short(const uint8_t**p)
{
    uint16_t r = *(uint16_t*)*p;
    (*p) += 2;
    return r;
}

static inline float get_float(const uint8_t**p)
{
    float r = *(float*)*p;
    (*p) += 4;
    return r;
}

static inline int64_t uncompress_int(const uint8_t**p)
{
    const uint8_t* b = (uint8_t*)*p;
    int sign = (*b & 0x40) ? -1 : 1;
    int64_t v = *b & 0x3F;
    int offset = 6;

    while (*b & 0x80) {
        b++;
        v += (*b & 0x7F) << offset;
        offset += 7;
    }

    b++;
    *p = b;
    return v * sign;
}

// 0000   b1 cb 74 00 0c 01 20 63 00 00 3e 05 52 00 52 00  ..t... c..>.R.R.
// 0010   b1 cb 01 00 00 30 30 30 30 30 31 87 07 b5 0e 00  .....000001.....
// 0020   02 05 44 ab a3 e3 0a f5 0e 92 b1 1a 03 c3 a5 43  ..D............C
// 0030   4d a7 d2 0e ab de 0b 00 a9 0b 41 00 9b 31 91 55  M.........A..1.U
// 0040   42 01 a6 5b 83 24 43 02 b0 47 92 4a 44 03 96 7f  B..[.$C..G.JD...
// 0050   ad 53 45 04 ae 47 8f 7c 48 18 01 01 47 05 15 00.SE..G. | H...G...
// 0060   87 07                                            ..

// 市场    代码    活跃度 现价 昨收    开盘    最高    最低    时间    保留    总量    现量    总金额    内盘    外盘    保留    保留    
// 买一价 卖一价    买一量    卖一量    买二价    卖二价    买二量    卖二量
// 买三价    卖三价    买三量    卖三量    买四价    卖四价    买四量    卖四量
// 买五价    卖五价    买五量    卖五量
// 保留    保留     保留     保留     保留     涨速 活跃度

//  1    000001    2870 3246.930000 3241.730000 3240.070000 3259.980000 3237.870000 14688307
// -2    190677550    0    225940963328.000000
// 106139369    106273072    30    320
// 35227.470000 3246.930000    703    497    4331.420000    3368.420000    22    2
// 897591.100000    3246.930000    584    676    3246.930000    3246.930000    0    0
// 3246.930000    3246.930000    0    0    2    0 
// -324693 -324693 -324693    0.060000    0

shared_ptr<MarketQuote> TdxHqApi::parse_quote(const uint8_t **pp)
{
    const uint8_t*p = *pp;
    int mkt = get_byte(&p);
    auto quote = make_shared<MarketQuote>();
    memset(quote.get(), 0, sizeof(MarketQuote));

    char code[7]; 
    memcpy(code, p, 6);
    code[6] = 0;

    if (mkt == 0)
        sprintf(quote->code, "%s.%s", code, "SZ");
    else if (mkt == 1)
        sprintf(quote->code, "%s.%s", code, "SH");
    else
        sprintf(quote->code, "%s.%d", code, mkt);

    double price_divisor = 100.0;

    bool has_askbid = true;
    auto it = m_codetable.find(quote->code);
    
    if (it != m_codetable.end()) {
        switch (it->second.price_precise) {
        case 0: price_divisor = 1.0;    break;
        case 1: price_divisor = 10.0;   break;
        case 2: price_divisor = 100.0;   break;
        case 3: price_divisor = 1000.0;   break;
        case 4: price_divisor = 10000.0;   break;
        case 5: price_divisor = 100000.0;   break;
        case 6: price_divisor = 1000000.0;   break;
        }
        has_askbid = it->second.has_askbid;
    }

    int64_t t = 0;
    p += 6;
    get_short(&p); // 活跃度
    int64_t base_price = uncompress_int(&p);
    quote->last         = base_price / price_divisor;
    quote->pre_close    = (uncompress_int(&p) + base_price) / price_divisor;
    quote->open         = (uncompress_int(&p) + base_price) / price_divisor;
    quote->high         = (uncompress_int(&p) + base_price) / price_divisor;
    quote->low          = (uncompress_int(&p) + base_price) / price_divisor;
    quote->time         = (uncompress_int(&p) + base_price) / price_divisor;
    t                   = uncompress_int(&p); 
    quote->volume       = uncompress_int(&p);
    t                   = uncompress_int(&p); // 现量
    quote->turnover     = get_float(&p);
    t                   = uncompress_int(&p); //内盘
    t                   = uncompress_int(&p); //外盘
    t                   = uncompress_int(&p); //保留
    t                   = uncompress_int(&p); //保留
    if (true) {
        quote->bid1         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->ask1         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->bid_vol1     =  uncompress_int(&p);
        quote->ask_vol1     =  uncompress_int(&p);
        quote->bid2         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->ask2         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->bid_vol2     =  uncompress_int(&p);
        quote->ask_vol2     =  uncompress_int(&p);
        quote->bid3         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->ask3         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->bid_vol3     =  uncompress_int(&p);
        quote->ask_vol3     =  uncompress_int(&p);
        quote->bid4         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->ask4         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->bid_vol4     =  uncompress_int(&p);
        quote->ask_vol4     =  uncompress_int(&p);
        quote->bid5         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->ask5         = (uncompress_int(&p) + base_price) / price_divisor;
        quote->bid_vol5     =  uncompress_int(&p);
        quote->ask_vol5     =  uncompress_int(&p);
    }
    else {
        for (int i = 0; i < 20; i++) uncompress_int(&p);
    }

    t                   = get_short(&p);//保留
    t                   = uncompress_int(&p);//保留
    t                   = uncompress_int(&p);//保留
    t                   = uncompress_int(&p);//保留
    t                   = uncompress_int(&p);//保留
    t                   = get_short(&p);//涨速 
    t                   = get_short(&p);//活跃度

    // check if time is correct!

    int h = quote->time / 10000;
    int m = (quote->time % 10000) / 100;
    int s = (quote->time % 100);
    bool update = false;
    if (s >= 60) { update = true; s = 59; }
    if (m >= 60) { update = true; m = 59; }
    if (h >= 24) { update = true; h = 23; } // Shouldn't happen!
    if (update) 
        quote->time = (h*10000 + m * 100 + s) * 1000;
    else
        quote->time *= 1000;

    *pp = p;
    return quote;
}

bool TdxHqApi::get_quotes(const vector<string>& codes, vector<shared_ptr<MarketQuote>>* quotes)
{
    //  0000   0c 01 20 63 00 02 13 00 13 00 3e 05 05 00 00 00  ..c......>.....
    //  0010   00 00 00 00 01 00 00 30 30 30 30 30 31           .......000001

    uint8_t head[] = {
        0x0c, 0x01, 0x20, 0x63, 0x00, 0x02, 0x28, 0x00, 0x28, 0x00,  // 0x28,0x00,0x28,0x00, data size
        0x3e, 0x05, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x04, 0x00 }; // code count

    int req_size = sizeof(head) + 7 * codes.size();
    uint8_t* req = new uint8_t[req_size];
    memset(req, 0, req_size);
    memcpy(req, head, sizeof(head));
    uint8_t *p = req + sizeof(head);

    int count = 0;
    for (int i = 0; i < codes.size(); i++) {
        const char* code = codes[i].c_str();
        const char* pdot = strchr(code, '.');

        if ( (pdot-code) <= 6 && pdot) {
            int mkt = -1;
            if (strcmp(pdot + 1, "SZ") == 0)
                mkt = 0;
            else if (strcmp(pdot + 1, "SH") == 0)
                mkt = 1;

            *p++ = mkt;
            memcpy(p, code, pdot - code);
            p += 6;
            count++;
        }
    }

    if (count == 0) {
        delete[] req;
        return false;
    }
    *(unsigned short*)(req + 6) = 12 + count * 7;
    *(unsigned short*)(req + 8) = 12 + count * 7;
    *(unsigned short*)(req + 20) = count;

    req_size = 22 + 7 * count;

    uint8_t* rsp = nullptr;
    int   rsp_size = 0;
    bool ret = false;
    if (call(req, req_size, &rsp, &rsp_size)) {

        const uint8_t* p = rsp;
        const uint8_t* p_end = rsp + rsp_size;

        // b1 cb
        // 01 00 -- quote count
        int count = *(unsigned short*)(p + 2);
        p += 4;

        while (p < p_end) {
            auto q = parse_quote(&p);
            if (q) {
                // Ignore empty quote
                if (q->time != 0) {
                    q->date = m_tradingday;
                    quotes->push_back(q);
                }
            }
        }

        ret = true;
    }
    if (rsp) delete[] rsp;

    delete[] req;

    return ret;
}


bool TdxHqApi::call(const uint8_t* req, int req_size, uint8_t** rsp, int* rsp_size)
{
    int r = ::send(m_sock, (const char*)req, req_size, 0);

    if (r != req_size) {
        cerr << "Send request error: " << r << endl;
        return false;
    }

    char head[16];
    r = ::recv(m_sock, head, 16, 0);
    if (r != 16) {
        cerr << "recv head error: " << r << endl;;
        return false;
    }

    static const uint8_t headdata[] = { 0xb1, 0xcb, 0x74, 0x00 };
    if (memcmp(head, headdata, 4) != 0) {
        cerr << "wrong response head" << endl;;
        return false;
    }

    unsigned short body_size = *(unsigned short*)(head + 12);
    unsigned short orig_size = *(unsigned short*)(head + 14);

    uint8_t* body = new uint8_t[body_size];
    int recv_size = 0;
    while (recv_size < body_size) {
        r = ::recv(m_sock, (char*)body + recv_size, body_size - recv_size, 0);
        if (r > 0) {
            recv_size += r;
        } else {
            cerr << "Recv body error " << body_size << "," << r << endl;;
            break;
        }
    }

    if (recv_size != body_size) {
        delete[] body;
        return false;
    }


    if (body_size != orig_size) {
        uint8_t* data = new uint8_t[orig_size];
        uLong data_len = orig_size;
        r = uncompress((uint8_t*)data, &data_len, body, body_size);
        if (r != Z_OK || data_len != orig_size) {
            cerr << "uncompress data error " << r << endl;;
            delete[] data;
            return false;
        }

        *rsp = data;
        *rsp_size = orig_size;
            
        delete body;
        return true;
    }
    else {
        *rsp = body;
        *rsp_size = orig_size;
        return true;
    }
}

int TdxHqApi::get_codetable_size(int mkt)
{
    // req
    //0000   0c 0c 18 6c 00 01 08 00 08 00                    ...l......
    //0000   4e 04 00 00 e7 53 33 01                          N....S3.

    // rsp
    //0000   b1 cb 74 00 0c 0c 18 6c 00 00 4e 04 02 00 02 00  ..t....l..N.....
    //0010   98 18    


    uint8_t head[] = { 
        0x0c, 0x0c, 0x18, 0x6c, 0x00, 0x01, 0x08, 0x00,
        0x08, 0x00, 0x4e, 0x04, 0x00, 0x00, 0xe7, 0x53,
        0x33, 0x01 };


    head[12] = mkt;

    uint8_t* rsp = nullptr;
    int rsp_size = 0;
    if (!call(head, sizeof(head), &rsp, &rsp_size)) {
        return -1;
    }

    int ret = rsp_size == 2 ? *(uint16_t*)rsp : -1;

    delete[] rsp;

    return ret;
}

shared_ptr<vector<Code>> TdxHqApi::get_codetable(int mkt)
{
    int count = get_codetable_size(mkt);
    if (count == -1) return nullptr;

    //0000   0c 01 18 64 01 01 06 00 06 00                    ...d......
    //0000   50 04 01 00 00 00                                P.....

    //e8 03 39 39 39 39 39 39 64 00 c9 cf d6 a4 d6 b8 ? .999999d. ? ? ? ? ? ?
    //ca fd 0c 58 0b 00 02 73 e0 49 45 00 00 00 00 39 ? ? .X...s ? IE....9
    //39 39 39 39 38 64 00 a3 c1 b9 c9 d6 b8 ca fd 6a  99998d. ? ? ? ? ? ? ? ? j
    //51 0b 00 02 4d 63 53 45 00 00 00 00 39 39 39 39  Q...McSE....9999
    //39 37 64 00 a3 c2 b9 c9 d6 b8 ca fd a2 06 00 00  97d. ? ? ? ? ? ? ? ? ? ...

    uint8_t head[] = {
        0x0c, 0x01, 0x18, 0x64, 0x01, 0x01, 0x06, 0x00,
        0x06, 0x00, 0x50, 0x04, 0x01, 0x00, 0x00, 0x00 };

    uint8_t* rsp = nullptr;
    int rsp_size = 0;

    head[12] = mkt;
    auto codes = make_shared<vector<Code>>();

    string mkt_code = mkt == 0 ? ".SZ" : ".SH";

    for (int i = 0; i < (count + 999) / 1000; i++){
        *(uint16_t*)(head + 14) = i * 1000;

        //LOG(INFO) << "Fetch code table " << mkt << " from " << i * 1000;
        rsp = nullptr;
        rsp_size = 0;
        if (!call(head, sizeof(head), &rsp, &rsp_size)) {
            break;
        }
        uint16_t code_count = *(uint16_t*)rsp;
        const uint8_t* p = rsp + 2;

        for (int n = 0; n < code_count; n++) {
            Code code;
            code.code = string(p, p + 6) + mkt_code;   p += 6;
            code.buy_size = *(uint16_t*)p;  p += 2;
            code.name = string(p, p + 8);   p += 8;
                                            p += 4; // reserve
            code.price_precise = *p;        p++;
            code.pre_close = *(float*)p;    p += 4;

            if (*(int16_t*)p == 0)
                code.has_askbid = false;
            else
                code.has_askbid = true;
            p += 2; // reserve
            p += 2; // reserve

            codes->push_back(code);
        }

        if (rsp) delete[] rsp;
    }

    return codes;
}

