
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
#ifndef CONFIG_INIAPI_H
#define CONFIG_INIAPI_H

#include <string>

#if defined(WIN32)
#ifdef INI_DLL
#define INI_API __declspec(dllexport)
#else
#define INI_API __declspec(dllimport)
#endif
#else
#define INI_API 
#endif


#include <vector>
#include <string>

namespace jzs
{
  using namespace std;

  class INI_API IniApi
  {
  public:
    static IniApi* CreateApi();
    
    // Return the result of ini_parse(), i.e., 0 on success, line number of
    // first error on parse error, or -1 on file open error.
    virtual bool Parse(const char *filename) = 0;

    // Get a string value from INI file, returning defval if not found.
    virtual void Get(const char *section, const char *name, const char *defval, string *ret) = 0;

    // Get a vector of string value from INI file, returning defval if not found.
    virtual void Get(const char *section, const char *name, const char *defval, string *ret, int& len) = 0;

    // Get an integer (long) value from INI file, returning defval if
    // not found or not a valid integer (decimal "1234", "-1234", or hex "0x4d2").
    virtual void Get(const char *section, const char *name, long defval, long &ret) = 0;

    // Get a real (floating point double) value from INI file, returning
    // defval if not found or not a valid floating point value
    // according to strtod().
    virtual void Get(const char *section, const char *name, double defval, double &ret) = 0;

    // Get a boolean value from INI file, returning defval if not found or if
    // not a valid true/false value. Valid true values are "true", "yes", "on", "1",
    // and valid false values are "false", "no", "off", "0" (not case sensitive).
    virtual void Get(const char *section, const char *name, bool defval, bool &ret) = 0;


    virtual void Release() = 0;
  };
}

//extern "C" INI_API dy::IniApi* NewIniApi(void *param);

#endif
