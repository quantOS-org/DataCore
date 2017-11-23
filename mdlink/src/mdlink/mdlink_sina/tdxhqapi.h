#ifndef _TDXHQAPI_H
#define _TDXHQAPI_H

#ifdef _WIN32

# define _WINSOCKAPI_

# include <Windows.h>
# include <WinSock2.h>
#else
typedef int SOCKET;
#  include <sys/socket.h>
#  include <netdb.h>
#  include <arpa/inet.h>
#  include <fcntl.h>
#endif

#include <map>
#include <stdint.h>
#include <memory>
#include <string>


namespace tdxhqapi {

    using namespace std;

    struct MarketQuote {
        char            code[32];
        int32_t         date;
        int32_t         time;
        double          open;
        double          high;
        double          low;
        double          close;
        double          last;
        //double          high_limit;
        //double          low_limit;
        double          pre_close;
        int64_t         volume;
        double          turnover;
        double          ask1;
        double          ask2;
        double          ask3;
        double          ask4;
        double          ask5;
        double          bid1;
        double          bid2;
        double          bid3;
        double          bid4;
        double          bid5;
        int64_t         ask_vol1;
        int64_t         ask_vol2;
        int64_t         ask_vol3;
        int64_t         ask_vol4;
        int64_t         ask_vol5;
        int64_t         bid_vol1;
        int64_t         bid_vol2;
        int64_t         bid_vol3;
        int64_t         bid_vol4;
        int64_t         bid_vol5;
    };

    struct Code {
        string code;
        string name;
        int buy_size;
        int price_precise;
        double pre_close;
        bool    has_askbid;
    };

    class TdxHqApi {
    public:

        TdxHqApi();
        ~TdxHqApi();

        static const int SZ = 0;
        static const int sh = 1;

        bool connect(const string&ip, int port);

        bool create_connection(const string& ip, int port);

        shared_ptr<vector<Code>> get_codetable(int mkt);

        bool get_quotes(const vector<string>& codes, vector<shared_ptr<MarketQuote>>* quotes);

        int get_tradingday() { return m_tradingday; }

        void get_server_addr(string* ip, int* port) {
            *ip = m_ip;
            *port = m_port;
        }

        void init_codetable(const vector<Code>& codes);

    private:
        bool login();

        int get_codetable_size(int mkt);

        bool call(const uint8_t* req, int req_size, uint8_t** rsp, int* rsp_size);

        shared_ptr<MarketQuote> parse_quote(const uint8_t **pp);

    private:
        SOCKET  m_sock;
        int     m_tradingday;;
        string  m_server_name;
        string  m_ip;
        int     m_port;
        map<string, Code> m_codetable;
    };


}


#endif

