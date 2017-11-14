
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
#include <map>
#include <string>
#include <cstring>
#include <algorithm>
#include "Ini.h"
#include "IniApi.h"


namespace jzs
{
  class IniApiImpl :public IniApi
  {
  public:
    IniApiImpl(){
    }

    ~IniApiImpl()
    {
    }

    inline void Release()
    {
      delete this;
    }

    inline bool Parse(const char *filename)
    {
      _error = ini_parse(filename, ValueHandler, this);
      return _error == 0;
    }

    inline void Get(const char *section, const char *name, const char *defval, string *ret)
    {
      string key = MakeKey(section, name);
      const char* val = _values.count(key) ? _values[key].c_str() : defval;

      *ret = string(val);
      printf("iniapi:%s, %s, [%s]\n", section, name, ret->c_str());
    }

    inline void Get(const char *section, const char *name, const char *defval, string* ret, int& len)
    {
      string key = MakeKey(section, name);
      const char *vals = _values.count(key) ? _values[key].c_str() : defval;
      printf("iniApi:%s, %s, [%s]\n", section, name, vals);

      int i = 0;
      char *p = strtok((char*)vals, "\n");
      while (i < len && p != NULL)
      {
        ret[i++] = string(p);
        p = strtok(NULL, "\n");
      }
      len = i;
    }

    inline void Get(const char *section, const char *name, long defval, long &ret)
    {
      string value;
      Get(section, name, "", &value);
      char* end;
      // This parses "1234" (decimal) and also "0x4D2" (hex)
      long n = strtol(value.c_str(), &end, 0);
      ret = end > value.c_str() ? n : defval;
    }

    inline void Get(const char *section, const char *name, double defval, double &ret)
    {
      string value;
      Get(section, name, "", &value);
      
      char* end;
      double n = strtod(value.c_str(), &end);
      ret = end > value.c_str() ? n : defval;
    }

    inline void Get(const char *section, const char *name, bool defval, bool &ret)
    {
      string value;
      Get(section, name, "", &value);  
      
      char* v = const_cast<char*>(value.c_str());
      
      for (size_t i = 0; i < strlen(v); i++)  v[i] = tolower(v[i]);

      if (!strcmp(v, "true") || !strcmp(v, "yes") || !strcmp(v, "on") || !strcmp(v, "1"))
        ret = true;
      else if (!strcmp(v, "false") || !strcmp(v, "no") || !strcmp(v, "off") || !strcmp(v, "0"))
        ret = false;
      else
        ret =defval;
    }


  private:
    static int ValueHandler(void* user, const char* section, const char* name, const char* value)
    {
      IniApiImpl* api = (IniApiImpl*)user;
      string key = MakeKey(section, name);
      if (api->_values[key].size() > 0)  api->_values[key] += "\n";
      api->_values[key] += value;
      return 1;
    }

    static std::string MakeKey(const std::string section, const std::string name)
    {
      string key = section + "." + name;
      // Convert to lower case to make section/name lookups case-insensitive
      std::transform(key.begin(), key.end(), key.begin(), ::tolower);
      return key;
    }

  private:
    int _error;
    std::map<std::string, std::string> _values;
  };

  IniApi* IniApi::CreateApi(){
    return new IniApiImpl();
  }
}

//extern "C" INI_API dy::IniApi *NewIniApi(void *param)
//{
//  return dy::IniApi::CreateApi();
//}
