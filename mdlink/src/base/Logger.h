
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
#ifndef BASE_LOGGER_H
#define BASE_LOGGER_H

#include <cstdio>
#include <cstdarg>
#include <string>
#include <glog/logging.h>
#include "config/SysConfig.h"

namespace jzs
{

    class Logger {
    public:
        static inline void init(const char* name)
        {
            google::InitGoogleLogging(name);

            std::string str;
        #ifdef _WIN32
            const char* p = strrchr(name, '\\');
        #else
            const char* p = strrchr(name, '/');
        #endif
            if (p)
                str = SysConfig::getDataDir() + std::string("/log/") + std::string(p + 1) + ".";
            else
                str = SysConfig::getDataDir() + std::string("/log/") + std::string(name) + ".";

            google::SetLogDestination(google::GLOG_INFO, str.c_str());
            FLAGS_logbuflevel = 1;
            FLAGS_alsologtostderr = true;
            //FLAGS_logtostderr = true;

            //FLAGS_minloglevel = 1;
            FLAGS_v = 1;
            FLAGS_colorlogtostderr = true;
            FLAGS_logbufsecs;
        }
    };
}





#endif
