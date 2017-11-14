
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
#ifndef BASE_TIMEUTIL_H
#define BASE_TIMEUTIL_H

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "StringUtil.h"

namespace jzs{
  
  using namespace std;
  
  typedef uint32_t Time;
  typedef uint32_t MillisTime;
  class TimeUtil
  {    

  public:    
    static const MillisTime millis_per_day = 24 * 60 * 60 * 1000;
    static const MillisTime millis_per_hour = 60 * 60 * 1000;
    static const MillisTime millis_per_minute = 60 * 1000;
    static const MillisTime night_begin_time_millis = 18 * 60 * 60 * 1000;
    static const MillisTime day_begin_time_millis = 6 * 60 * 60 * 1000;
    static const MillisTime MidNightSwitchBegin = (23 * 60 * 60 + 59 * 60 + 50) * 1000;
    static const MillisTime MidNightSwitchEnd = 10 * 1000;
    
    static int compare_time(const MillisTime& t1, const MillisTime& t2)
    {
        return compare_time_new(t1, t2);
    }    

    inline static MillisTime time_add(MillisTime t1, MillisTime t2)
    {
        return (t1 + t2) % millis_per_day;
    }

    inline static MillisTime time_add(MillisTime t1, MillisTime t2, bool& day_switched)
    {
        MillisTime t = t1 + t2;
        if (t >= millis_per_day) {
            day_switched = true;
            return t % millis_per_day;
        }
        else{
            day_switched = false;
            return t;
        }
    }

    // make sure t1 > t2 by compare_time function
    inline static MillisTime time_sub(MillisTime t1, MillisTime t2)
    {
        if (t1 < t2) {
            return t1 + millis_per_day - t2;
        }
        else {
            return t1 - t2;
        }
    }

    inline static MillisTime time_sub(MillisTime t1, MillisTime t2, bool& day_switched)
    {
        if (t1 < t2) {
            day_switched = true;
            return t1 + millis_per_day - t2;
        }
        else {
            day_switched = false;
            return t1 - t2;
        }
    }
    //HHMMSSmmm 
    static MillisTime TimeToMillis(Time time) {
      int time_new, temp;
      temp = time % 1000; 
      time_new = temp;  // ms
      time /= 1000;
      temp = time % 100; 
      time_new += temp * 1000;  //s
      time /= 100;
      temp = time % 100; 
      time_new += temp * 60000; //m
      time /= 100;
      time_new += time * 3600000;//h    
      return time_new;
    }
    
    static int time_to_hhmsMS(int time) {
        int ms = time % 1000;
        time /= 1000;
        int s = time % 60;
        time /= 60;
        int m = time % 60;
        int h = time / 60;

        return h * 10000 * 1000 + m * 100 * 1000 + s * 1000 + ms;
    }
    
    static MillisTime hhmmss(char *hhmmss, int millis){
      int hh, mm, ss;
      sscanf(hhmmss, "%d:%d:%d", &hh, &mm, &ss);
      return (hh * 3600 + mm * 60 + ss) * 1000 + millis;
    }

    static int ctp_time_to_int(char *hhmmss, int millis){
        int hh, mm, ss;
        sscanf(hhmmss, "%d:%d:%d", &hh, &mm, &ss);
        return (hh * 10000 + mm * 100 + ss) * 1000 + millis;
    }

    //YYYYMMDDhhmmss
    static char* datetimestr() 
    {
      static char datetime[15];

      time_t t = time(NULL);
      struct tm *tm = localtime(&t);  
      strftime(datetime, 15, "%Y%m%d%H%M%S", tm);
      return datetime;
    }
    
    static char* datestr(){
      static char datetime[15];

      time_t t = time(NULL);
      struct tm *tm = localtime(&t);  
      strftime(datetime, 15, "%Y%m%d", tm);
      return datetime;
    }
    
    static char* timestr(){
      static char datetime[15];

      time_t t = time(NULL);
      struct tm *tm = localtime(&t);  
      strftime(datetime, 15, "%H%M%S", tm);
      return datetime;
    }
    static char* timestr_minute(int time)
    {
        static char minute_time[10];
        int tmp = time / 1000; // seconds
        tmp /= 100; // minutes
        int mm = tmp % 100;
        tmp /= 100; // hours;
        int hh = tmp;
        sprintf(minute_time, "%02d:%02d", hh, mm);
        return minute_time;
    }
    static char* timestr_seconds(int time)
    {
        static char minute_time[10];
        int tmp = time / 1000; // seconds
        int ss = tmp % 100;
        tmp /= 100; // minutes
        int mm = tmp % 100;
        tmp /= 100; // hours;
        int hh = tmp;
        sprintf(minute_time, "%02d:%02d:%02d", hh, mm, ss);
        return minute_time;
    }
    
    static char* datestr(int date)
    {
        static char date_time[15];
        int yy = date / 10000;
        int mm = date % 10000 / 100;
        int dd = date % 100;
        
        sprintf(date_time, "%d-%02d-%02d", yy, mm, dd);
        return date_time;
    }

    static int date()
    {
      time_t t = time(NULL);
      struct tm *tm=localtime(&t);
      return (tm->tm_year + 1900) * 10000 + (tm->tm_mon + 1) * 100 + tm->tm_mday;
    }

    static MillisTime dayMillis()
    {
      chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();

      time_t tnow = std::chrono::system_clock::to_time_t(now);
      tm *date = localtime(&tnow);
      date->tm_hour = 0;
      date->tm_min = 0;
      date->tm_sec = 0;
      auto midnight = chrono::system_clock::from_time_t(mktime(date));

      return (int)chrono::duration_cast<std::chrono::milliseconds>(now - midnight).count();
    }

    static void parse_timestamp(const string& v, int* date, int* time)
    {
        std::tm tm;
#ifdef _WIN32
        std::stringstream ss(v);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
#else
        strptime(v.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
#endif
        if (date) *date = (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
        if (time) *time = (tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec)*1000;

        vector<string> strs = split(v,".");        
        if (strs.size() == 2) {
            int millisecs = atoi(strs[1].c_str());
            if (time) {
                *time = (*time)  + millisecs;
            }
        }
    }    
    

    static bool isInNightTime(MillisTime time)
    {
        return !(time > TimeUtil::day_begin_time_millis && time < TimeUtil::night_begin_time_millis);
    }    
    
    static Time MillisToTime(MillisTime time)
    {
        int s = time / 1000;
        return ((s / 3600) * 10000 + (s % 3600) / 60 * 100 + s % 60) * 1000 + time % 1000;
    }

    static string MillisTimeStrMm(MillisTime time)
    {
        return TimeStrMm(MillisToTime(time));
    }

    static string TimeStrMm(Time time)
    {
        return string(timestr_minute(time));
    }

    static string MillisTimeStrSs(MillisTime time)
    {
        return TimeStrSs(MillisToTime(time));
    }

    static string TimeStrSs(Time time)
    {
        return string(timestr_seconds(time));
    }

    static string MillisTimeStrMs(MillisTime time)
    {
        return TimeStrMs(MillisToTime(time));
    }

    static string TimeStrMs(Time time)
    {
        static char time_str[15];
        int ms = time % 1000; // ms
        int tmp = time / 1000; // seconds
        int ss = tmp % 100;
        tmp /= 100; // minutes
        int mm = tmp % 100;
        tmp /= 100; // hours;
        int hh = tmp;
        sprintf(time_str, "%02d:%02d:%02d:%03d", hh, mm, ss,ms);
        return string(time_str);
    }

  private:
      static int compare_time_night(const MillisTime& t1, const MillisTime& t2);
      static int compare_time_new(const MillisTime& t1, const MillisTime& t2);
    
  };

}

#endif
