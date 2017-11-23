#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <fstream>
#include "tdxhqapi.h"


using namespace std;
using namespace std::chrono;

using namespace tdxhqapi;


int main()
{

#ifdef _WIN32
    {
        WORD ver = MAKEWORD(2, 2);
        WSADATA wdata;
        WSAStartup(ver, &wdata);
    }
#endif

    TdxHqApi api;

    //if (!api.connect("121.14.110.200", 443)) {
    if (!api.connect("180.153.39.51", 7709)) {
        //if (!api.connect("58.215.43.187", 7709)) {
        cerr << "Connect server failed";
        return -1;
    }


    // 取代码表并初始化 api 内部的价格精度


    const char* mkt_code[] = { "SZ", "SH" };

    vector<Code> total_codes;

    for (int mkt = 0; mkt < 2; mkt++) {
        auto codetable = api.get_codetable(mkt);
        if (codetable) {
            cout << "Got code table : SZ " << codetable->size() << endl;
            for (auto& c : *codetable) total_codes.push_back(c);
        }

        ofstream out;
        stringstream ss; ss << "ct_" << mkt_code[mkt] << "_" << api.get_tradingday() << ".csv";
        string filename = ss.str();
        cout << "save codetable to " << filename << endl;;
        out.open(filename, ios::trunc);
        if (out.is_open()) {
            out << "code,name,buy_size,price_precise,pre_close\n";
            for (auto& c : *codetable)
                out << c.code << "," << c.name << "," << c.buy_size << "," << c.price_precise << "," << c.pre_close << endl;
        }
    }

    // 代码表中有每个代码的价格小数点位数，如果不设置，缺省为2位
    api.init_codetable(total_codes);


    if (1){
        vector<shared_ptr<MarketQuote>> quotes;
        vector<string> codes;
        for (size_t i = 0; i < 200;  i++){
            codes.push_back(total_codes[i].code);
        }     
     
        while (true) {
            quotes.clear();
            auto t1 = chrono::system_clock::now();
            if (api.get_quotes(codes, &quotes)){
                for (auto q : quotes) {
                    cout << "quote: " << q->code << "|" << q->date << "|" << q->time << "|" << q->last << "|" <<
                        q->volume << "|" << q->turnover << "|" <<
                        q->bid1 << "|" << q->ask1 << "|" << q->ask2 << "|" << q->bid2 << endl;;
                }
            }
            auto t2 = chrono::system_clock::now();
            cout << "====================================================================================" << endl;
            cout << "t2 - t1 = " << (t2 - t1).count() << endl;
            this_thread::sleep_for(seconds(10));
        }
    }

    return 0;
}
