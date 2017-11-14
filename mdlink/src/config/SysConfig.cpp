
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
#include <iostream>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include "SysConfig.h"
#include "base/Logger.h"
#include "base/StringUtil.h"
#include "json/json.h"
#include "CXML.h"
#include "XMLAction.h"
#include "base/TimeUtil.h"
#ifdef __linux__
static
size_t get_executable_path( char* processdir,char* processname, size_t len)
{
    char* path_end;
    if(readlink("/proc/self/exe", processdir,len) <=0)
        return -1;
    path_end = strrchr(processdir,  '/');
    if(path_end == NULL)
        return -1;
    ++path_end;
    strcpy(processname, path_end);
    *path_end = '\0';
    return (size_t)(path_end - processdir);
}
#endif

namespace jzs
{

    MdlinkCfg                   SysConfig::md;
    QmsCfg                      SysConfig::qms;
    std::string                 SysConfig::m_home_dir;
    std::string                 SysConfig::m_etc_dir;
    std::string                 SysConfig::m_data_dir;
    GlobalCfg                   SysConfig::m_globalcfg;

    class SysConfigLoader : public SysConfig
    {
    public:
        static void init();
        static void Load_global(const Json::Value& global);
        static void Load_qms(const Json::Value& global);
        static bool Load_mdlink(string mdname, MdlinkCfg& cfg);
    };

    void SysConfig::init()
    {
        SysConfigLoader::init();
    }

    bool SysConfig::getMdlinkCfg(string mdname, MdlinkCfg& cfg)
    {
        return SysConfigLoader::Load_mdlink(mdname, cfg);
    }


    bool SysConfigLoader::Load_mdlink(string mdname, MdlinkCfg& cfg)
    {
        ifstream is;
        string jzs_config = m_etc_dir + "/jzs.json";
        is.open(jzs_config.c_str(), ios::binary);
        Json::Value root;
        Json::Reader reader;
        LOG(INFO) << "Load mdlink config file from " << jzs_config;

        if (reader.parse(is, root)){
            LOG(INFO) << "Json Read Succeed!";
            const Json::Value& mdlink_cfgs = root["mdlink"];
            if (!mdlink_cfgs.empty()) {
                const Json::Value& mdlink = mdlink_cfgs[mdname];
                if (!mdlink.empty()) {
                    cfg.addr = mdlink["pub_addr"].asString();
                    cfg.id = mdname;
                    cfg.route = mdlink["route"].asString();

                    if (mdlink["time_tolerance"].empty()) {
                        // Tolenrance time for time check. The unit is millisecond. 
                        cfg.time_tol = 500;
                    }
                    else  {
                        // Tolenrance time for time check. The unit is millisecond.
                        cfg.time_tol = mdlink["time_tolerance"].asInt();
                    }

                    if (!mdlink["source_id"].empty()) {
                        cfg.source_id = mdlink["source_id"].asInt();
                    }
                    else {
                        cfg.source_id = 0;
                    }

                    LOG(INFO) << "Mdlink Name: " << mdname << endl;
                    LOG(INFO) << "PubAddr: " << mdlink["pub_addr"].asString() << endl;
                    for (int i = 0; i < mdlink["sources"].size(); i++) {
                        MdCfg svrcfg;
                        const Json::Value& source = mdlink["sources"][i];
                        svrcfg.id = source["id"].asString();
                        svrcfg.route = source["route"].asString();
                        LOG(INFO) << "\tSource: " << source["id"].asString() << " Type " << source["route"].asString();
                        if (cfg.route == "CTP" || cfg.route == "LTS") {
                            LOG(INFO) << "\tfront: " << endl;
                            for (int j = 0; j < source["front"].size(); j++) {
                                svrcfg.front.push_back(source["front"][j].asString());
                                LOG(INFO) << "\t\t" << source["front"][j].asString() << endl;
                            }
                            svrcfg.broker = source["broker"].asString();
                            svrcfg.investor = source["investor"].asString();
                            svrcfg.passwd = source["passwd"].asString();
                            svrcfg.udp = source["udp"].asBool();
                            svrcfg.multicast = source["multicast"].asBool();
                            svrcfg.do_publish = source["do_publish"].asBool();
                            svrcfg.broadcast = source["broadcast"].asString();
                            svrcfg.port = source["port"].asInt();
                            LOG(INFO) << "\tbroker: " << source["broker"].asString() << endl;
                            LOG(INFO) << "\tinvestor: " << source["investor"].asString() << endl;
                            LOG(INFO) << "\tpasswd: " << source["passwd"].asString() << endl;
                            LOG(INFO) << "\tudp: " << source["udp"].asBool() << endl;
                            LOG(INFO) << "\tmulticast: " << source["multicast"].asBool() << endl;
                        }
                        else if (cfg.route == "MERGE") {
                            svrcfg.addr = source["addr"].asString();
                            LOG(INFO) << "\tAddr: " << source["addr"].asString() << endl;
                        }
                        else if (cfg.route == "TDF") {
                            svrcfg.addr = source["addr"].asString();
                            svrcfg.port = source["port"].asInt();
                            svrcfg.username = source["username"].asString();
                            svrcfg.passwd = source["passwd"].asString();
                            svrcfg.markets = source["markets"].asString();
                            LOG(INFO) << "\taddr: " << source["addr"].asString() << endl;
                            LOG(INFO) << "\tport: " << source["port"].asInt() << endl;
                            LOG(INFO) << "\tusername: " << source["username"].asString() << endl;
                            LOG(INFO) << "\tpasswd: " << source["passwd"].asString() << endl;
                            LOG(INFO) << "\tmarkets: " << source["markets"].asString() << endl;
                        }
                        cfg.mdvec.push_back(svrcfg);
                    }
                    md = cfg;
                    return true;
                }
                else {
                    LOG(ERROR) << mdname << " config is not found !" << endl;
                }
            }
            else {
                LOG(ERROR) << "Mdlink config is not found !" << endl;
            }
        }
        else {
            LOG(ERROR) << "Json Read Failed!" << endl;
        }
        return false;
    }
    void SysConfigLoader::init()
    {
#ifdef __linux__
        const char* p = getenv("JZS_HOME");
        if (!p || strlen(p)==0) {
            char path[PATH_MAX];
            char processname[1024];
            get_executable_path(path, processname, sizeof(path));

            for (int i = 0; i<2; i++) {
                char* path_end = strrchr(path,  '/');
                if (path_end != NULL) {
                    *path_end = '\0';
                }
            }
            m_home_dir = string(path);
        } else {
            m_home_dir = p;
        }

        std::cout<<"Use JZS home: " << m_home_dir << endl;

        int rc = chdir(m_home_dir.c_str());
        if (rc==0){
            std::cout<<"Set working directory to "<<m_home_dir<<endl;
        }else {
            perror("Set working directory failed:");
            LOG(FATAL);
        }
#else
        const char* p = getenv("JZS_HOME");
        if (!p || strlen(p) == 0) {
            char path[MAX_PATH];
            GetModuleFileName(NULL, path, MAX_PATH);
            for (int i = 0; i < 2; i++) {
                char* path_end = strrchr(path, '\\');
                if (path_end != NULL) {
                    *path_end = '\0';
                }
            }
            m_home_dir = string(path);
        }
        else {
            m_home_dir = p;
        }

        m_home_dir = trim(m_home_dir);

        std::cout << "Use JZS home: " << m_home_dir << endl;

        BOOL  rc = SetCurrentDirectory(m_home_dir.c_str());
        if (rc){
            std::cout << "Set working directory to " << m_home_dir << endl;
        }
        else {
            perror("Set working directory failed:");
            LOG(FATAL);
        }
#endif
        m_etc_dir = m_home_dir + "/etc";
        m_data_dir = m_home_dir + "/data";

        string jzs_config = m_etc_dir + "/jzs.json";

        ifstream is;
        is.open(jzs_config.c_str(), ios::binary);
        Json::Value jroot;
        Json::Reader jreader;

        if (jreader.parse(is, jroot)) {
            Load_global(jroot["global"]);
            Load_qms(jroot["qms"]);
        }
    }

    void SysConfigLoader::Load_global(const Json::Value& global)
    {
        if (!global.empty()) {
            const Json::Value& md_addr = global["mdlink_addr"];
            if (!md_addr.empty()) {
                m_globalcfg.mdlink_addr = md_addr.asString();
            }
            const Json::Value& qms_addr = global["qms_addr"];
            if (!qms_addr.empty()) {
                m_globalcfg.qms_addr = qms_addr.asString();
            }
        }
    }

    void SysConfigLoader::Load_qms(const Json::Value& qms_config)
    {
        if (!qms_config.empty()) {
            qms.id = qms_config["id"].asString();
            qms.check_code = qms_config["check_code"].asBool();
            qms.addr = qms_config["addr"].asString();
            if (qms_config["bar_type"].empty()) {
                qms.bar_type = 60;
            }
            else {
                qms.bar_type = qms_config["bar_type"].asInt();
            }

            if (qms_config["bar_offset"].empty()) {
                qms.bar_offset = 3;
            }
            else {
                qms.bar_offset = qms_config["bar_offset"].asInt();
            }

            if (qms_config["save_tk"].empty()) {
                qms.save_tk = true;
            }
            else {
                qms.save_tk = qms_config["save_tk"].asBool();
            }
        }
    }

}
