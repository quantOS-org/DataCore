
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
#ifndef _JZS_PERF_H
#define _JZS_PERF_H

#include <time.h>
#ifdef _WIN32
#include <Windows.h>
#endif

class PerfSensorData {
public:
    int         count;
    uint32_t    ticks;
    uint32_t    max_ticks;
    uint32_t    min_ticks;
    
    PerfSensorData() :
        count(0),
        ticks(0),
        max_ticks(0),
        min_ticks(0xFFFFFFFF)
     {
     }
};

class PerfSensor {
public:
    PerfSensor(PerfSensorData* data) : m_data(data)
    {
        m_tickcount = get_tickcount();
    }
    ~PerfSensor() {
        uint32_t ticks = (uint32_t)(get_tickcount() - m_tickcount);
        m_data->count++;
        m_data->ticks += ticks;
        if (ticks > m_data->max_ticks) m_data->max_ticks = ticks;
        if (ticks < m_data->min_ticks) m_data->min_ticks = ticks;
    }
    uint64_t m_tickcount;
    PerfSensorData* m_data;
    
    static uint64_t get_tickcount() {
#ifdef __linux__
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#else
        return GetTickCount64();
#endif
    }
};

#define PERF_SENSOR( _NAME_ ) \
        extern PerfSensorData _perf_sensor_data_ ## _NAME_;  \
        PerfSensor _perf_sensor ##_NAME_( \
            &_perf_sensor_data_ ## _NAME_); \

#define PERF_DATA(_NAME_)  PerfSensorData _perf_sensor_data_ ## _NAME_
    

#endif
