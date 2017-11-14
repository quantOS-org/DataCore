
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

package org.quantos.jads.services.jsi

import scala.collection.mutable.ArrayBuffer
import akka.actor.{Actor, ActorRef}
import akka.actor.Props
import org.slf4j.LoggerFactory
import org.quantos.jads.Config
import org.quantos.jads.common.DataType.ColumnSet
import org.quantos.jads.gateway.JRpcServer.{JsonCallReq, JsonCallRsp}
import org.quantos.jads.utils.{BaseData, MathUtil, TimeUtil}
import org.quantos.jads.utils.TimeUtil._
import org.quantos.utils.jrpc.JsonHelper
import jzs.api.QmsClient
import org.quantos.jads.services.jsi.jsiUtils.Bar

import scala.collection.mutable

object JsiService {
 
    
    object InitReq

    lazy val actor = Config.akka.system.actorOf(Props[JsiService], "jsiServer")

    case class JsiQueryReq (
                               symbol     : String,
                               fields     : String,
                               freq       : String,
                               begin_time : Int = jsiUtils.default_begin,
                               end_time   : Int = jsiUtils.default_end,
                               trade_date : Int = 0
    )

    case class JsiForwardReq (clientActor: ActorRef, client : String, req : JsiQueryReq, bar_view:Boolean)

    def start() {
        actor ! InitReq
    }
}

class JsiService extends Actor{

    import JsiService._

    val logger = LoggerFactory.getLogger("JsiServer")

    val qms = new QmsClient

    def receive = {

        case InitReq => init()

        case JsonCallReq(client, method, params) => onJsonCallReq(client, method, params)

        case x => println ("Unknown msg: " + x)
    }

    def init() {
        logger.info("Start JSI Service")
        if(Config.conf.qms != null) {
            qms.connect(Config.conf.qms.addr)
        }
    }

    def onJsonCallReq(client: String, method: String, params: Any) {

        try {
            if (method == "jsi.query") {
                onQueryReq(sender(), client, params, false)
            }
            else if(method == "jsi.bar_view") {
                onQueryReq(sender(), client, params, true)
            }
            else
                sender() ! JsonCallRsp(null, message="unkown method: " + method)
        } catch {
            case t: Throwable =>
                t.printStackTrace()
                sender() ! JsonCallRsp(null, message=t.getMessage)
        }
    }

    def onQueryReq(clientActor: ActorRef, client: String, params: Any, bar_view: Boolean) {

        val req = JsonHelper.convert[JsiQueryReq](params)
    
        val (ds, err_msg) = query(if (req.symbol != null) req.symbol else "",
                                  if (req.fields != null) req.fields else "",
                                  if (req.begin_time != null) req.begin_time else jsiUtils.default_begin,
                                  if (req.end_time != null) req.end_time else jsiUtils.default_end,
                                  if (req.freq != null) req.freq else "1M",
                                  bar_view
                                 )

        clientActor ! JsonCallRsp( ds, err_msg, if (ds != null) 0 else -1)
    }
    
    

    def query(securitys: String, fields: String, begin_time: Int, end_time: Int,
              cycle: String, bar_view: Boolean): (ColumnSet, String) = {
        // -2 error
        // -1 empty
        
        if (securitys == "" )
            return (null, "wrong symbol")
        
        val codes = securitys.split("[,]") map {_.trim } filter (_ != "")
    
        if (codes.size == 0 ) {
            return (null, "wrong symbol")
        }
        val bars = ArrayBuffer[Bar]()
    
        if (cycle != "1M" && cycle != "5M" && cycle != "15M") {
            return (null,  "wrong freq, only support 1M, 5M and 15M")
        }
        
        for(security <- codes) {
    
            val bar1m = qms.getBar1M(security)
    
            if (bar1m != null) {
    
                // 万得宏汇的指数行情中 TURNOVER单位为 百元。这里复原成元
                // TODO：应该在 mdlink 中修改！
                var turnover_mul = 1
                val inst = BaseData.getInstrument(security)
                if (inst != null &&
                    (inst.insttype == 100 && (inst.market == 1 || inst.market == 2)))
                    turnover_mul = 100
                val is_fut = (inst.insttype == 101 ||
                    inst.insttype == 102 ||
                    inst.insttype == 103)
                val is_index = inst.insttype==100
                val last = bar1m.getLast
                val trade_date = BaseData.getCurTradeDate()
                for (i <- 0 until bar1m.getBarCount) {
                    val b = bar1m.getBar(i)
                    val date = b.getDate
                    val time = b.getTime.toHumanSecondTime
                    
                    if (TimeUtil.compareTimeSec(time, begin_time) >= 0 &&
                        TimeUtil.compareTimeSec(time, end_time) <= 0 ) {
                        if(last != null) {
                            bars += Bar(
                                SYMBOL        = security,
                                CODE          = inst.instcode,
                                DATE          = date,
                                TRADEDATE     = trade_date,
                                CYCLE         = cycle,
                                TIME          = time,
                                OPEN          = b.getOpen,
                                HIGH          = b.getHigh,
                                LOW           = b.getLow,
                                CLOSE         = b.getClose,
                                VOLUME        = b.getVolumeInc.toLong,
                                TURNOVER      = b.getTurnoverInc * turnover_mul,
                                VWAP          = if(is_index) Double.NaN else b.getVwap,
                                SETTLE        = Double.NaN,
                                OPENINTEREST  = if(is_fut) b.getInterest else Double.NaN,
                                ASK1          = last.getAb.getAskPrice(0),
                                ASK2          = last.getAb.getAskPrice(1),
                                ASK3          = last.getAb.getAskPrice(2),
                                ASK4          = last.getAb.getAskPrice(3),
                                ASK5          = last.getAb.getAskPrice(4),
                                BID1          = last.getAb.getBidPrice(0),
                                BID2          = last.getAb.getBidPrice(1),
                                BID3          = last.getAb.getBidPrice(2),
                                BID4          = last.getAb.getBidPrice(3),
                                BID5          = last.getAb.getBidPrice(4),
                                ASKVOL1       = last.getAb.getAskVolume(0),
                                ASKVOL2       = last.getAb.getAskVolume(1),
                                ASKVOL3       = last.getAb.getAskVolume(2),
                                ASKVOL4       = last.getAb.getAskVolume(3),
                                ASKVOL5       = last.getAb.getAskVolume(4),
                                BIDVOL1       = last.getAb.getBidVolume(0),
                                BIDVOL2       = last.getAb.getBidVolume(1),
                                BIDVOL3       = last.getAb.getBidVolume(2),
                                BIDVOL4       = last.getAb.getBidVolume(3),
                                BIDVOL5       = last.getAb.getBidVolume(4))
                        } else {
                            bars += Bar(
                                SYMBOL        = security,
                                CODE          = inst.instcode,
                                DATE          = date,
                                TRADEDATE     = date,
                                CYCLE         = cycle,
                                TIME          = time,
                                OPEN          = b.getOpen,
                                HIGH          = b.getHigh,
                                LOW           = b.getLow,
                                CLOSE         = b.getClose,
                                VOLUME        = b.getVolumeInc.toLong,
                                TURNOVER      = b.getTurnoverInc * turnover_mul,
                                VWAP          = b.getVwap,
                                SETTLE        = Double.NaN,
                                OPENINTEREST  = b.getInterest.toLong)
                        }
                    }
                }
            }
        }

        val ds = 
            if (cycle != "1m" && cycle != "1M" && bars.nonEmpty ) {
                jsiUtils.toColumnSet(buildBar(bars, cycle), fields, bar_view)
            }
            else 
                jsiUtils.toColumnSet( bars, fields,bar_view)
        ( ds, null)
    }

    
    /**
     * "2016-06-01 21:49:00" -> (20160601, 214900)
     */
    def parseDateTime( str: String): (Int, Int) = {
        
        val ss = str.trim.split(' ').map { _.trim } filter { _ != "" }
        
        if (ss.length > 2 )
            return ( -2, -2)

        var date = -1
        var time = -1
        
        ss foreach { s =>
            if (s.contains('-') && date == -1){
                val tmp = s.split('-')
                if (tmp.length != 3) return (0,0)
                date = tmp(0).toInt * 10000 + tmp(1).toInt * 100 + tmp(2).toInt
            } else if ( s.contains(':') && time == -1) {
                val tmp = s.split(':')
                if (tmp.length != 3) return (0,0)

                time = tmp(0).toInt * 10000 + tmp(1).toInt * 100 + tmp(2).toInt
            } else {
                return (-2, -2)
            }
        }
        
        (date, time)
    }


    def calcBarTime(date: Int, time: Int, barSize: Int) : (Int, Int) = {
        
        val minute = time / 10000 * 60 + ((time / 100) % 100)
        var barTime = (minute / barSize + 1) * barSize
        if (barTime < 24*60) {
            val h = barTime / 60
            val m = barTime % 60
            (date,  (h* 10000 + m * 100).toInt)
        } else {
            barTime -= 24 * 60
            val h = barTime / 60
            val m = barTime % 60
            (date.nextDay,  (h* 10000 + m * 100).toInt)
        }
    }
    
    def max(x: Double, y:Double) =  if ( x > y) x else y

    def min(x: Double, y:Double) =  if ( x < y) x else y

    def buildBar(bars : Seq[Bar], cycle: String) : Seq[Bar] = {

        val new_bars = new ArrayBuffer[Bar]()
    
        var barSize = 1
        if(cycle == "5M" ) {
            barSize = 5
        } else {
            barSize = 15
        }
            
        var minutes = barSize*100
        
        var t = calcBarTime(bars.head.DATE, bars.head.TIME, barSize)
        var DATE = t._1
        var TIME = t._2
        val SYMBOL        : String = bars.head.SYMBOL
        val CODE          : String = bars.head.CODE
        val TRADEDATE     : Int    = bars.head.TRADEDATE
        var OPEN          : Double = bars.head.OPEN
        var HIGH          : Double = bars.head.HIGH
        var LOW           : Double = bars.head.LOW
        var CLOSE         : Double = bars.head.CLOSE
        var VOLUME        : Double = bars.head.VOLUME
        var TURNOVER      : Double = bars.head.TURNOVER
        var OPENINTEREST  : Double = bars.head.OPENINTEREST

        for ( i <- 1 until bars.size) {
            var bar = bars(i)

            t = calcBarTime(bar.DATE, bar.TIME, barSize)

            if ( t._1 != DATE || t._2 != TIME ) {
                new_bars += Bar(
                    SYMBOL        = SYMBOL,
                    CODE          = CODE,
                    DATE          = DATE,
                    TRADEDATE     = TRADEDATE,
                    CYCLE         = cycle,
                    TIME          = TIME,
                    OPEN          = OPEN     ,
                    HIGH          = HIGH     ,
                    LOW           = LOW      ,
                    CLOSE         = CLOSE    ,
                    VOLUME        = VOLUME   ,
                    TURNOVER      = TURNOVER ,
                    VWAP          = TURNOVER/VOLUME,
                    SETTLE        = Double.NaN,
                    OPENINTEREST  = OPENINTEREST)
                DATE          = t._1    
                TIME          = t._2
                OPEN          = bar.OPEN    
                HIGH          = bar.HIGH    
                LOW           = bar.LOW     
                CLOSE         = bar.CLOSE   
                VOLUME        = bar.VOLUME
                TURNOVER      = bar.TURNOVER
                OPENINTEREST  = bar.OPENINTEREST

            } else {
                HIGH          = max(bar.HIGH, HIGH)    
                LOW           = min(bar.LOW, LOW)     
                CLOSE         = bar.CLOSE   
                VOLUME        += bar.VOLUME  
                TURNOVER      += bar.TURNOVER
                OPENINTEREST  = bar.OPENINTEREST
            }
        }
        new_bars += Bar(
            SYMBOL        = SYMBOL,
            CODE          = CODE,
            DATE          = DATE,
            TRADEDATE     = TRADEDATE,
            CYCLE         = cycle,
            TIME          = TIME,
            OPEN          = OPEN     ,
            HIGH          = HIGH     ,
            LOW           = LOW      ,
            CLOSE         = CLOSE    ,
            VOLUME        = VOLUME   ,
            TURNOVER      = TURNOVER ,
            VWAP          = TURNOVER/VOLUME,
            SETTLE        = Double.NaN,
            OPENINTEREST  = OPENINTEREST)
        
        new_bars
    }
    
   
    


    
}

