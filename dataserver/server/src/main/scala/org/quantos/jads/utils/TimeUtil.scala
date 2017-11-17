
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

package org.quantos.jads.utils

import java.sql.Timestamp
import java.time.LocalDate
import java.util.{Calendar, Date}

object TimeUtil {
    
    val night_begin_time = 190000
    val night_begin_day_time = 19 * 60 * 60 * 1000
    val day_begin_time = 60000
    val day_begin_day_time = 6 * 60 * 60 * 1000
    
    def diff(t1: Int, t2: Int):Int = {
        if(t1 > t2) {
            return 1
        } else if(t1 < t2) {
            return -1
        } else {
            return 0
        }
    }
    
    def isNightTime(time:Int): Boolean = {
        if(time >= night_begin_time ) {
            return true
        } else {
            return false
        }
    }
    
    
    def compareDayTime(t1: Int, t2: Int): Int = {
        if(t1 >= night_begin_day_time) {
            if(t2 >= night_begin_day_time) {
                diff(t1,t2)
            } else {
                -1
            }
        } else {
            if(t2 < night_begin_day_time) {
                diff(t1,t2)
            } else {
                1
            }
        }
    }
    
    def compareTimeSec(t1: Int, t2: Int): Int = {
        if(t1 < 0) {
            return -1
        }
        
        if(t2 < 0) {
            return 1
        }
        
        if(t1 >= 999999) {
            return 1
        }
        
        if(t2 >= 999999) {
            return -1
        }
            
        if(t1 >= night_begin_time) {
            if(t2 >= night_begin_time) {
                diff(t1,t2)
            } else {
                -1
            }
        } else {
            if(t2 < night_begin_time) {
                diff(t1,t2)
            } else {
                1
            }
        }
    }

    implicit class TimeConverter(var v:Long) {

        def hmsms = {
            val ms = v % 1000
            v /= 1000
            val hms = for ( i <-1 to 3 ) yield ( v % 100, v /= 100)
            ms + (hms(0)._1 + hms(1)._1*60 + hms(2)._1*3600) * 1000
        }

        def hms = {
            val hms = for ( i <-1 to 3 ) yield ( v % 100, v /= 100)
            (hms(0)._1 + hms(1)._1*60 + hms(2)._1*3600) * 1000
        }

        def seconds = v * 1000

        def millis = v

        def minuts = v * 60 * 1000

        def hours =  v * 3600 * 1000

        def toLocalTime = (v + java.util.TimeZone.getDefault.getOffset(0)) % ( 3600*24*1000)

        def toDate = {
            // FIXME: later
            val date = new java.util.Date( v / ( 3600*24*1000) * 3600*24*1000)
            val year = date.getYear + 1900
            val month = date.getMonth + 1
            val day = date.getDate
            f"$year$month%02d$day%02d".toInt
        }

        def toLocalDate = {
            (v + java.util.TimeZone.getDefault.getOffset(0)).toDate
        }

        def toHMSMS: Int = {
            val ms = v % 1000
            v /= 1000
            val t = ((v % 60) + ((v/60)%60)*100 + (v/3600) * 10000) *1000+ ms
            t.toInt
        }

        def toHumanSecondTime: Int = {
            val h = v / (3600*1000)
            val m = (v /  (60*1000)) % 60
            val s = (v / 1000) % 60

            (h* 10000 + m * 100 + s).toInt
        }

        private val intDateSdf = new java.text.SimpleDateFormat("yyyyMMdd")

        def nextDay : Int = {
            val date = intDateSdf.parse(v.toString())
            val c = Calendar.getInstance
            c.setTime(date)
            c.add(Calendar.DATE, 1)
            intDateSdf.format( c.getTime ).toInt
        }

    }

    implicit  class TimerstampConverter(val time: Timestamp){
        def local = {
            (time.getTime + java.util.TimeZone.getDefault.getOffset(0)) % ( 3600*24*1000)
        }

        def toHumanDate(): Int = {
            intDateSdf.format( time ).toInt
        }
    }

    implicit class DateConverter(val date: Date) {

    }
    private val intDateSdf = new java.text.SimpleDateFormat("yyyyMMdd")
    private val intTimeSdf = new java.text.SimpleDateFormat("HHmmss")

    /**
     * Get Today as an integer, format is yyyyMMdd
     */
    def todayAsInt() = intDateSdf.format( java.util.Calendar.getInstance.getTime).toInt
    def nowTimeAsInt() = intTimeSdf.format( java.util.Calendar.getInstance.getTime).toInt
    
    def getLocalDateFromInt(date:Int) = {
        val year  = date/10000
        val month = date / 100 % 100
        val day   = date % 100
        val dt = LocalDate.of(year,month,day)
        dt
    }
    
    def getIntFromLocalDate(date:LocalDate) = {
        val dt = date.getYear * 10000 + date.getMonthValue * 100 + date.getDayOfMonth
        dt
    }
  
}