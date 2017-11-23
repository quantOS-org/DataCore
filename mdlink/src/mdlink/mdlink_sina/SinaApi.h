#ifndef _SINA_API_H
#define _SINA_API_H


#include <memory>
#include <string>
#include <vector>
#include "curl.h"

namespace sina_api {

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
        double          high_limit;
        double          low_limit;
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


    class SinaApi {
    public:

        SinaApi();

        ~SinaApi();
        void set_url(string url) { m_url = url; }
        bool get_quotes(const vector<string>& codes, vector<shared_ptr<MarketQuote>>* quotes);

    private:
        string download(const vector<string>& codes);

        CURL * m_curl;
        string m_url;
    };


}


#endif

