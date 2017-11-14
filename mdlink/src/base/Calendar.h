
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
#ifndef CALENDAR_H
#define CALENDAR_H

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include "TimeUtil.h"
using namespace jzs;
using namespace std;
namespace jzs {
    class Calendar
    {
    public:
        Calendar();
        static Calendar* getInst();
        int GetActionDayFromTradeDay(int tradeday, MillisTime time, int jzcode = -1);
        int GetTradeDayFromActionDay(int actionday, MillisTime time, int jzcode = -1);
        int GetTradeDay(MillisTime time, int jzcode = -1);
        int GetPreDay(int day);
        int GetNextDay(int day);
        int GetDate();
        void init();
        ~Calendar();
    private:
        void GetNextDay(struct tm& td, struct tm& tm);
        void GetPreDay(struct tm& td, struct tm& tm);
        void GetNextTime(struct tm& today, struct tm& tommrow, long seconds);
        void GetPreTime(struct tm& today, struct tm& tommrow, long seconds);
        int GetWeekDay(int yymmdd);

        // Next work day
        void GetNextWorkDay(struct tm& td, struct tm& tm)
        {
            GetNextDay(td, tm);
            struct tm tmp;
            while (tm.tm_wday == 0 || tm.tm_wday == 6) {
                tmp = tm;
                GetNextDay(tmp, tm);
            }
        }
        std::vector<int> m_tradedays;
    };
}
#endif
