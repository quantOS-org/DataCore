
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
#include <unistd.h>
#include <fstream>

#include "Init.h"
#include "Logger.h"
#include "config/SysConfig.h"


namespace jzs {
    
    using namespace std;

    static void save_pid(const char* prog_name)
    {
#ifdef __linux__
        const char* path_end;
        path_end = strrchr(prog_name,  '/');
        if(path_end)
            ++path_end;
        else
            path_end = prog_name;
        
        string file = SysConfig::getDataDir() + "/tmp/" + path_end + ".pid";
        ofstream out;
        out.open(file, ios::trunc | ios::out);
        if (out.is_open())
            out<< getpid();
#endif
    }
    
    void init(const char* prog_name)
    {

        SysConfig::init();

        Logger::init(prog_name);
      
        save_pid(prog_name);
    }
}


