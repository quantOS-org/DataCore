
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
/*
 * File:   Config.h
 * Author: dxb
 *
 * Created on June 22, 2015, 2:52 PM
 */

#ifndef JZS_SYSCONFIG_H
#define    JZS_SYSCONFIG_H

#include <string>
#include <vector>
#include <map>

namespace jzs
{
    using namespace std;

    struct MonitorSymbol{
        string symbol;
        int max_delay;

        MonitorSymbol(string s, int d) : symbol(s), max_delay(d)
        {}
    };

    struct MdCfg {
        string id;
        string route;
        vector<string> front; //vector
        string broker;
        string investor;
        string passwd;
        string broadcast;
        string authcode;

        bool udp;
        bool multicast;
        bool do_publish;       
        string filter;

        string addr;
        int port;
        string username;
        string markets;
        
    };

    struct MdlinkCfg {
        string id;
        string addr;
        string route;
        vector<MdCfg> mdvec;
        int time_tol; // ms
        int source_id; // source id;
    };

    struct QmsCfg {
        string id;
        string addr;
        bool check_code; 
        bool save_tk;
        int bar_type; // bar size, unit is second; for example, 60 stands for the 1 minute bar
        int bar_offset; // bar begin time shift, unit is second 
    };

    struct GlobalCfg {
        string qms_addr;
        string mdlink_addr;
    };
    
    class SysConfig
    {
    public:

        static bool getMdlinkCfg(string mdname, MdlinkCfg& cfg);

        static QmsCfg* getQmsCfg()
        {
            return &qms;
        }

        static const string& getHomeDir()      { return m_home_dir; }
        static const string& getEtcDir()       { return m_etc_dir;  }
        static const string& getDataDir()      { return m_data_dir; }
        
        static const GlobalCfg* getGlobalCfg()      { return &m_globalcfg; }

        static void init();

    protected:
        static GlobalCfg   m_globalcfg;
        static MdlinkCfg   md;
        static QmsCfg qms;
        static std::string m_home_dir;
        static std::string m_etc_dir;
        static std::string m_data_dir;
    };

}

#endif    /* CONFIG_H */

