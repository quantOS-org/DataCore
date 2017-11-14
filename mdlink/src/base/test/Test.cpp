
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
#include "DynamicClass.h"
#include "ini/IniAPi.h"
#include "msg/MarketBook.h"


using namespace dy;

void mainxx(int argc, char *argv[])
{
  DynamicClass *dc = DynamicClass::Load("ini.dll");
  IniApi *api = (IniApi *)dc->NewObject("NewIniApi", NULL);
  api->Parse("feedclient.ini");
  
  long port;
  api->GetInteger("CLIENT", "port", 0, port);

  printf("port=%d\n", port);
  api->Release();
  dc->Free();

  printf("sizeof(MarketBook) =%d\n", sizeof(MarketBook));
}