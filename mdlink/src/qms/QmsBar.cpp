
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
#include "base/DoubleUtil.h"
#include "base/TimeUtil.h"
#include "base/Calendar.h"
#include "QmsBar.h"
#include "Qms.h"

#ifdef __linux__
#include "shared_ptr_atomic.h"
#endif
namespace jzs {

 
    void Bar1M_MakeupTo(QuoteData* qd, int bar_time)
    {
        if (bar_time < 0) {
            LOG(ERROR) << "Makeup time is negative: " << bar_time;
            return;
        }
        const MarketQuote& quote = qd->last;

        if (!quote.has_qs()) {
            return;
        }

        MarketInfo* mkt_info = qd->mkt;
        vector<Bar>& bar_list = qd->bar_1m;
        bool has_last_bar = false;
        int begin_bartime = mkt_info->get_begin_time(quote.jzcode());

        int last_bar_id = -1;
        if (bar_list.size() != 0) {
            has_last_bar = true;
            last_bar_id = bar_list.size() - 1;
            begin_bartime = bar_list[last_bar_id].bar_time;
        }
        int tmp_time = TimeUtil::time_add(begin_bartime, bar_period);
        while (TimeUtil::compare_time(tmp_time, bar_time) <= 0) {
            bool is_auct = mkt_info->is_aucttime(TimeUtil::time_sub(tmp_time, 1000),
                quote.jzcode());
            if (is_auct)  {
                bar_list.push_back(Bar());
                Bar* bar = &bar_list[bar_list.size() - 1];
                bar->bar_time = tmp_time;
                bar->date = g_tables->g_calendar->GetActionDayFromTradeDay(quote.qs().tradeday(), tmp_time, quote.jzcode());
                if (has_last_bar) {
                    bar->open = bar->high = bar->low = bar->close = bar_list[last_bar_id].last->last();
                    bar->vwap = bar_list[last_bar_id].last->vwap();
                    bar->last = bar_list[last_bar_id].last;

                    bar->volume_total = bar_list[last_bar_id].last->volume();
                    bar->turnover_total = bar_list[last_bar_id].last->turnover();
                    bar->interest_total = bar_list[last_bar_id].last->interest();
                }
                else {
                    //之前没有bar数据，用preclose补齐
                    //TODO: get preclose from DB fo makeup bar;
                    bar->high = bar->low = bar->open = bar->close = quote.qs().preclose();
                    bar->vwap = quote.vwap();
                    bar->last = nullptr;
                }
                bar->match_item = 0;
                bar->setFlag(Bar::BarFlag::AutoCompleted);

                //  bar->flag = 0x01;
                bar->is_auct = is_auct;
                qd->last_bartime = bar->bar_time;
                qd->last_bardate = bar->date;
            }            
            tmp_time = TimeUtil::time_add(tmp_time, bar_period);
        }
    }

    void Bar1M_Insert(QuoteData* qd)
    {        
        const MarketQuote& quote = qd->last;       
        MarketInfo* mkt_info = qd->mkt;
        vector<Bar>& bar_list = qd->bar_1m;
        int bar_date = quote.qs().date();
        auto quote_ptr = shared_ptr<MarketQuote>(new MarketQuote(quote));
        //*quote_ptr = quote;
        // WARNING:
        //  严格按照交易时间生成 bar1m
        //  如果要把集合竞价、休市阶段的数据生成BAR，需要检查所有调用 Bar1m_MakeupTo代码

        bool is_auct = mkt_info->is_aucttime(quote.time(), quote.jzcode());
        if (!is_auct) return;
        
        // 收市最后一分钟的行情不能带到下个交易时段
        MillisTime bar_time;
        MillisTime offset_time = bar_offset;
        bool dayadd = false;
        MillisTime tmp_time = TimeUtil::time_add(quote.time(), offset_time, dayadd);
        
        if (dayadd) {
            bar_date = g_tables->g_calendar->GetNextDay(bar_date);
        }
        tmp_time %= TimeUtil::millis_per_day;
        if (mkt_info->is_aucttime(tmp_time, quote.jzcode()))
            bar_time = ((tmp_time) / (bar_period)+1) * (bar_period);
        else
            bar_time = (quote.time() / (bar_period)+1) * (bar_period);
        if (bar_time >= TimeUtil::millis_per_day) {
            // switch to midnight
            bar_time %= TimeUtil::millis_per_day;
            bar_date = g_tables->g_calendar->GetNextDay(bar_date);
        }
        bar_date = g_tables->g_calendar->GetActionDayFromTradeDay(quote.qs().tradeday(), bar_time, quote.jzcode());
        Bar* bar = nullptr;
        Bar* last_bar = nullptr;
        if (bar_list.size() > 0) {
            last_bar = &bar_list[bar_list.size() - 1];
            if (last_bar->bar_time == bar_time) {
                bar = last_bar;
            }
            else if (bar_date < last_bar->date ||
                (bar_time < last_bar->bar_time && bar_date == last_bar->date)) {
                // 插入时间小于bar最新时间
                LOG_EVERY_N(ERROR, 20)
                    << jz_symbol(qd->last.jzcode())
                    << " wrong bar time, "
                    << " quote time: " << TimeUtil::time_to_hhmsMS(quote.time())
                    << " new  bar time: " << TimeUtil::time_to_hhmsMS(bar_time) << " "<<bar_date 
                    << " last bar time: " << TimeUtil::time_to_hhmsMS(last_bar->bar_time) << " " <<last_bar->date;
                return;
            }
        }

        if (bar ) {            
            if (bar->last) {
                if (quote.last() > bar->high)  bar->high = quote.last();
                if (quote.last() < bar->low)   bar->low = quote.last();
                bar->close = quote.last();
                bar->vwap = quote.vwap();
                bar->volume_inc += quote.volume() - bar->last->volume();
                bar->turnover_inc += quote.turnover() - bar->last->turnover();
                bar->interest_inc += quote.interest() - bar->last->interest();
            }
            else {
                // last may be null when last bar added by makeup 
                bar->open = bar->high = bar->low = bar->close = quote.last();
                bar->vwap = quote.vwap();
                bar->volume_inc = quote.volume();
                bar->turnover_inc = quote.turnover();
                bar->interest_inc = quote.interest();
            }
            bar->volume_total = quote.volume();
            bar->turnover_total = quote.turnover();
            bar->interest_total = quote.interest();
            bar->match_item = 0;
            bar->cleanFlag();            
            bar->last = quote_ptr;
        }
        else {

            if (qd->last.jzcode() == 8) {
                LOG(INFO) << "000001.SH NEW BAR " << TimeUtil::time_to_hhmsMS(bar_time);
            }
            if ((!last_bar) || (last_bar &&
                (TimeUtil::time_add(last_bar->bar_time, bar_period) != bar_time))) {
                if (qd->last.jzcode() == 8 || qd->last.jzcode() == 25229) {
                    if (last_bar) {
                        LOG(ERROR) << "Bar1M_MakeupTo " << jz_symbol(qd->last.jzcode())
                            << " " << TimeUtil::time_to_hhmsMS(last_bar->bar_time) / 1000
                            << " -> " << TimeUtil::time_to_hhmsMS(bar_time) / 1000;
                    }
                    else {
                        LOG(ERROR) << "Bar1M_MakeupTo " << jz_symbol(qd->last.jzcode())
                            << "begin time " << " -> " <<
                            TimeUtil::time_to_hhmsMS(bar_time) / 1000;
                    }
                }
                Bar1M_MakeupTo(qd, TimeUtil::time_sub(bar_time, bar_period));
            }
            
            bar_list.push_back(Bar());
            bar = &bar_list.back();
            bar->date = bar_date;
            bar->bar_time = bar_time;
            bar->open = bar->high = bar->low = bar->close = quote.last();
            bar->vwap = quote.vwap();
            if (bar_list.size() > 1 && bar_list[bar_list.size() - 2].last) {
                auto last = bar_list[bar_list.size() - 2].last;
                bar->volume_inc = quote.volume() - last->volume();
                bar->turnover_inc = quote.turnover() - last->turnover();
                bar->interest_inc = quote.interest() - last->interest();
            }
            else {
                bar->volume_inc = quote.volume();
                bar->turnover_inc = quote.turnover();
                bar->interest_inc = quote.interest();
            }

            bar->volume_total = quote.volume();
            bar->turnover_total = quote.turnover();
            bar->interest_total = quote.interest();

            bar->match_item = 0;
            bar->last = quote_ptr;

            bar->is_auct = is_auct;
            qd->last_bartime = bar_time;
            qd->last_bardate = bar->date;
        }
        if (mkt_info->last_bardate < bar->date ||
            (MillisTime)(mkt_info->last_bartime) < bar_time && mkt_info->last_bardate == bar->date) {
            mkt_info->last_bartime = bar_time;
            mkt_info->last_bardate = bar->date;
        }
    }

}