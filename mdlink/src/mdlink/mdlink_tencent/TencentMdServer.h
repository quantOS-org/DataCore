#ifndef JZS_SINAMDSERVER_H
#define JZS_SINAMDSERVER_H
#include <vector>
#include <string>

#include "../public/MdServer.h"
#include "protocol/cpp/md.pb.h"
#include "TencentApi.h"

namespace jzs
{

    using namespace std;
    using namespace ::jzs::msg;
    using namespace std::chrono;  

    class TencentMdServer : public MdServer
    {

    private:
        MdCfg m_cfg;
        std::thread* m_thread;
        std::vector<std::string> m_subscribed;
        vector<string> m_query_codes;
        unordered_map<string, shared_ptr<tencent_api::MarketQuote> > m_market_quotes;
        unordered_map<string, Instrument> m_tencent_inst_map;

        bool init_by_type(MdlinkCfg& mdcfg);
        void Start() override;
        bool tencent_to_jzs(shared_ptr<tencent_api::MarketQuote> md, MarketQuote* pbk);
        bool is_same_quote(shared_ptr<tencent_api::MarketQuote> md1,
            shared_ptr<tencent_api::MarketQuote> md2);

    public:
        TencentMdServer() {};
        ~TencentMdServer() {};
        void Run();

    };

}

#endif