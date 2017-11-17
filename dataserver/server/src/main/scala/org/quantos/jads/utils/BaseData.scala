
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

import java.text.SimpleDateFormat
import java.util.Calendar

import com.fasterxml.jackson.annotation.{JsonIgnoreProperties, JsonPropertyOrder}
import com.fasterxml.jackson.dataformat.csv.CsvSchema
import org.quantos.jads.Config
import org.slf4j.LoggerFactory

import scala.collection.mutable.{ArrayBuffer, ListBuffer}
import scala.concurrent.Await
import scala.concurrent.duration.Duration
import scala.io.Source
import scala.util.control.Breaks


object BaseData {

    // calendar.csv
    // date
    case class Date(date : Int)

    // market.csv
    // market,nationcode,timezone,marketcode,auctbeg1,auctend1,auctbeg2,auctend2,auctbeg3,auctend3,auctbeg4,auctend4
    case class Market(market: Int, timezone: Int, marketcode: String)

    // instrument.csv
    // instcode,jzcode,targetjzcode,market,buylot,selllot,pricetick,multiplier,insttype,symbol

    @JsonIgnoreProperties(ignoreUnknown = true)
    @JsonPropertyOrder(Array("instcode","jzcode","targetjzcode","market","buylot","selllot",
        "pricetick","multiplier","insttype","symbol"))
    case class Instrument (jzcode: Int,
                           targetjzcode: Int,
                           instcode: String,
                           symbol : String,
                           insttype: Int,
                           market: Int,
                           buylot: Double,
                           selllot: Double,
                           pricetick: Double,
                           multiplier: Int
                          )

    class Symbol(val jzcode: Int, val symbol: String) { }

    var symbol_inst_map = Map[String, Instrument]()   // symbol -> instrument
    var jzcode_inst_map = Map[Int, Instrument]()    // jzcode -> instrument
    private var calendar  = ArrayBuffer[Int] ()
    private var trade_date = 0;

    val logger = LoggerFactory.getLogger("basedata")
    
    def init() {
        // Ensure MySQL driver can be loaded.

        try {
            loadInstrument()
            loadCalendar()
            val start = System.currentTimeMillis()
            val end = System.currentTimeMillis()
            println(s"Runtime of DB load: %d".format(end-start))

        } catch {
            case e: Throwable => e.printStackTrace
        }
    }
    
    def loadCalendar(): Unit = {

        val path = "etc/calendar.csv"
        val schema = CsvSchema.emptySchema().withHeader()
        val dates  = CsvHelper.deserialize[Date]( Source.fromFile(path).mkString)

        calendar ++= dates.map{_.date}

        val today = TimeUtil.todayAsInt()
        val time = TimeUtil.nowTimeAsInt()
        trade_date = getTradeDate(today,time)
    }
    
    def getCurTradeDate(): Int = {
        return trade_date
    }
    
    def getTradeDate(date:Int, time:Int): Int = {
        if(TimeUtil.isNightTime(time)) {
            return getNextTradeDate(date)
        } else {
            return getNextOrCurTradeDate(date)
        }
    }
    
    def getTradeDates(begin: Int, end: Int): ListBuffer[Int] = {
        var dates = ListBuffer[Int] ()
        for( date <- calendar) {
            if(date >= begin ) {
                if(date <= end) {
                    dates += date
                } else {
                    return dates
                }
            }
        }
        return dates
    }
    
    def getNextTradeDate(date:Int): Int = {
        for(dt<-calendar) {
            if(dt > date) {
                return dt
            }
        }
        return 0
    }
    
    def getNextOrCurTradeDate(date:Int): Int = {
        for(dt<-calendar) {
            if(dt >= date) {
                return dt
            }
        }
        return 0
    }
    
    
    def getPreOrCurTradeDate(date:Int): Int = {
        for(i<-calendar.indices) {
            val dt = calendar(i)
            if(dt > date) {
                if(i>0) {
                    return calendar(i-1)
                } else {
                    return 0
                }
            }
        }
        return 0
    }
    
    def getPreTradeDate(date:Int): Int = {
        for(i<-calendar.indices) {
            val dt = calendar(i)
            if(dt >= date) {
                if(i>0) {
                    return calendar(i-1)
                } else {
                    return 0
                }
            }
        }
        return 0
    }
    
    def isFuture(inst: Instrument): Boolean = {
        inst.insttype match {
            case 101 => true
            case 102 => true
            case 103 => true
            case _ => false
        }
    }
    
    def isBond(inst: Instrument) :Boolean = {
        if(inst != null) {
            inst.insttype match {
                case 8 => true
                case 10 => true
                case 11 => true
                case 12 => true
                case 13 => true
                case 14 => true
                case 15 => true
                case 16 => true
                case 17 => true
                case 18 => true
                case 19 => true
                case 20 => true
                case _  => false
            }
        } else {
            false
        }
    }
    
    def isIndex(inst: Instrument) :Boolean = {
        if(inst != null) {
            inst.insttype match {
                case 100 => true
                case _   => false
            }
        } else {
            false
        }
    }

    def loadInstrument() {

        val path = "etc/instrument.csv"
        val schema = CsvSchema.emptySchema().withHeader()
        val insts  = CsvHelper.deserialize[Instrument]( Source.fromFile(path).mkString)

        jzcode_inst_map = insts map { x => (x.jzcode -> x) } toMap

        symbol_inst_map = insts map { x => (x.symbol -> x) } toMap

        logger.info("Load {} active instruments in {}", jzcode_inst_map.size, TimeUtil.todayAsInt())
    }

    def getJzCode(symbol: String): Int = {
        val t = symbol_inst_map.getOrElse(symbol, null)
        if ( t != null )
            t.jzcode
        else
            -1
    }

    def getInstrument(symbol: String): Instrument = {
        val t = symbol_inst_map.getOrElse(symbol, null)
        if ( t != null ) {
           jzcode_inst_map.getOrElse(t.jzcode, null)
        } else {
            logger.error("Can't find symbol " + symbol)
            null
        }
    }

    def getInstrument(jzcode: Int): Instrument = {
        jzcode_inst_map.getOrElse(jzcode, null)
    }
    

    def getSymbol(jzcode: Int, default: String = ""): String = {
        val s = jzcode_inst_map.getOrElse(jzcode, null)
        if (s != null) s.symbol
        else default
    }

    def getAllSymbols() = symbol_inst_map.keys

}

