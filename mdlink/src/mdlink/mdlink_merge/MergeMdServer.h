
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
#ifndef JZS_Merge_MdServer_H
#define JZS_Merge_MdServer_H

#include <thread>
#include <string>
#include <set>
#include <map>
#include "protocol/cpp/md.pb.h"
#include "base/Logger.h"
#include "../public/MdServer.h"
#include "msgbus/Subscriber.h"
#include <queue>
#include <mutex>
#include <condition_variable>
namespace jzs{
    using namespace std;
     
    class MdClient : public msgbus::SubscriberCallback {
    public:
        MdClient(const MdCfg* cfg);
        virtual ~MdClient();  

        void Start() ;        
        virtual void on_topic(const jzs::msg::Msg& cont) override;

    private:
        void forward_publish(const jzs::msg::Msg& msg);
        void Run();
        
    private:
        const MdCfg* m_cfg;        
        msgbus::Subscriber*  m_up_mdlink;    
        std::thread* m_thread;
    };

    class MergeMdServer : public MdServer
    {
    public:
        MergeMdServer() : 
            m_exec(false), 
            m_thread(nullptr) 
        { ; }
        ~MergeMdServer();
        void set_stop() { m_exec = false; }

    private:
        void Run();
        virtual bool init_by_type(MdlinkCfg& mdcfg);
        virtual void Start() override;
        virtual void publish(jzs::msg::md::MarketDataType type,
                             const jzs::msg::md::MarketQuote& bk);
        
        bool m_exec;
      
        std::thread* m_thread;     
        vector<MdClient*> m_clients;
    };
}

#endif

