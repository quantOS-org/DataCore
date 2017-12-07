#include <cstring>
#include <fstream>
#include <thread>
#include <chrono>
#include <memory>
#include <iomanip>
#include <set>
#include "config/IniApi.h"
#include "config/SysConfig.h"
#include "base/StringUtil.h"
#include "base/Logger.h"
#include "base/Init.h"
#include "base/TimeUtil.h"
#include "protocol/cpp/md.pb.h"
#include <unistd.h>
#include <getopt.h>
#include <json/json.h>
#include <mutex>
#include <unordered_map>
#include <set>
#include "TencentMdServer.h"
#include "TencentApi.h"
#include "protocol/cpp/md.pb.h"
#include "base/Calendar.h"
#include "../public/MapTables.h"


using namespace std;
using namespace jzs;
using namespace jzs::msg;
using namespace jzs::msg::md;
using namespace jzs::msgbus;
using namespace tencent_api;
void test_tencent()
{
	string url_test = "http://qt.gtimg.cn/q=";
    tencent_api::TencentApi api;
	api.set_url(url_test);

    vector<shared_ptr<tencent_api::MarketQuote>> quotes;

    vector<string> codes;
    codes.push_back("sz000001");
    codes.push_back("sh000001");
    codes.push_back("sh502028");
    codes.push_back("sz399001");
    api.get_quotes(codes, &quotes);

	cout << "test tencent\n";
    for (auto q : quotes) {
        cout << q->code << "," << q->date << "," << q->time << "," << q->last << "," << q->volume << endl;
    }
    getchar();
}

int main(int argc, char* argv[])
{
	string mdid;
    if (argc == 2) {
        mdid = string(argv[1]);
        init(argv[1]);
    }
    else {
        init(argv[0]);
        LOG(ERROR) << "Wrong number of arguments given!" << endl;
        return -1;
    }

    MdlinkCfg mdcfg;
    SysConfig::getMdlinkCfg(mdid, mdcfg);
    TencentMdServer* svr = new TencentMdServer;
    if (!svr->init(mdcfg)) {
        return -1;
    }

    svr->StartMdlink();
    while (true) {
        this_thread::sleep_for(chrono::seconds(5));
        auto next_time = chrono::system_clock::now() + chrono::seconds(5);
        while (chrono::system_clock::now() < next_time)
            this_thread::sleep_until(next_time);
		svr->ShowStatus();
    }
    return 0;
}

