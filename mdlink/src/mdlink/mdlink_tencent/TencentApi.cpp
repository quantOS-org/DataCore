#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <string.h>
#include <iostream>
#include <vector>
#include <curl.h>

#include "base/StringUtil.h"
#include "TencentApi.h"


using namespace std;
using namespace std::chrono;
using namespace tencent_api;


TencentApi::TencentApi()
{
    m_curl = curl_easy_init();
}

TencentApi::~TencentApi()
{
    curl_easy_cleanup(m_curl);
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) 
{
    string data((const char*)ptr, (size_t)size * nmemb);
    *((stringstream*)stream) << data << endl;
    return size * nmemb;
}

string TencentApi::download(const vector<string>& codes)
{
    stringstream ss;
    ss << m_url;
    for (int i = 0; i < codes.size(); i++) {
        ss << codes[i];
        if (i + 1 != codes.size())
            ss << ",";
    }

    string url = ss.str();
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1); //Prevent "longjmp causes uninitialized stack frame" bug
    curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "deflate");
    curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPALIVE , 1L);
    curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPIDLE  , 1L);
    curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPINTVL , 1L);
    std::stringstream out;
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &out);

    CURLcode res = curl_easy_perform(m_curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
    }

    return out.str();
}

static inline int parse_date(const string& date)
{
	if (date.length() != 8) return 0;
	return atoi(date.c_str());
}

static inline int parse_time(const string& time)
{
	if (time.length() != 6) return 0;
	return atoi(time.c_str()) *1000;
}

static inline shared_ptr<MarketQuote> parse_quote(const string& raw_line)
{
    vector<string> kv;
    string line = remove_substr(raw_line, "\r");
    line = remove_substr(line, "\n");
    split(line, "=", kv);
    if (kv.size() != 2) {
        return nullptr;
    }
    auto quote = make_shared<MarketQuote>();
    memset(quote.get(), 0, sizeof(quote));

    {
        vector<string> tmp;
        split(kv[0], "_", tmp);
        string code = tmp.back();
    //    strncmp(quote->code, code.c_str(), 32);
        strcpy(quote->code, code.c_str());   
    }
    
    if (kv[1] == "\"\"") {
        return nullptr;
    }

    {
        vector<string> tmp;
        split(kv[1], "\"", tmp, false);

        string values_str = tmp[1];
        vector<string> values;

        split(values_str, "~", values);
        if (values.size() == 0) {
            return nullptr;
        }
		int i = 3;
		quote->last = atof(values[i++].c_str());
		quote->pre_close = atof(values[i++].c_str());
		quote->open = atof(values[i++].c_str());
		quote->volume = atoll(values[i++].c_str()) * 100;
		i += 2; // ignore the outer, inner

		quote->bid1 = atof(values[i++].c_str());
		quote->bid_vol1 = atoi(values[i++].c_str()) * 100;
		quote->bid2 = atof(values[i++].c_str());
		quote->bid_vol2 = atoi(values[i++].c_str()) * 100;
		quote->bid3 = atof(values[i++].c_str());
		quote->bid_vol3 = atoi(values[i++].c_str()) * 100;
		quote->bid4 = atof(values[i++].c_str());
		quote->bid_vol4 = atoi(values[i++].c_str()) * 100;
		quote->bid5 = atof(values[i++].c_str());
		quote->bid_vol5 = atoi(values[i++].c_str()) * 100;

		quote->ask1 = atof(values[i++].c_str());
		quote->ask_vol1 = atoi(values[i++].c_str()) * 100;
		quote->ask2 = atof(values[i++].c_str());
		quote->ask_vol2 = atoi(values[i++].c_str()) * 100;
		quote->ask3 = atof(values[i++].c_str());
		quote->ask_vol3 = atoi(values[i++].c_str()) * 100;
		quote->ask4 = atof(values[i++].c_str());
		quote->ask_vol4 = atoi(values[i++].c_str()) * 100;
		quote->ask5 = atof(values[i++].c_str());
		quote->ask_vol5 = atoi(values[i++].c_str()) * 100;
		i += 1; // ignore last transactions

		quote->date = atoi(values[i].substr(0, 8).c_str());
		quote->time = atoi(values[i++].substr(8, 6).c_str())*1000;
		i += 2; // ignore ups and downs

        quote->high     = atof(values[i++].c_str());
        quote->low      = atof(values[i++].c_str());
        i += 2; // ignore transaction, vol

        quote->turnover = atoll(values[i++].c_str())*10000;

    }
    return quote;
}


bool TencentApi::get_quotes(const vector<string>& codes, vector<shared_ptr<MarketQuote>>* quotes)
{
    string content = download(codes);

    vector<string> lines;
    split(content, ";", lines);
    for (string line : lines) {
        if (strncmp(line.c_str(), "v", 1) != 0) continue;        
        auto q = parse_quote(trim(line));
        if (q)
            quotes->push_back(q);
    }
    return true;
}

