
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

import org.quantos.jads.common.DataType._

import scala.collection.mutable

/**
  * Created by xzhou on 2017/9/28.
  */
object jsiUtils {
    
    val default_begin = 200000
    val default_end   = 170000
    
    
    case class Bar(
                      SYMBOL        : String,
                      CODE          : String,
                      DATE          : Int,
                      TRADEDATE     : Int,
                      CYCLE         : String,
                      TIME          : Int,
                      OPEN          : Double,
                      HIGH          : Double,
                      LOW           : Double,
                      CLOSE         : Double,
                      VOLUME        : Double,
                      TURNOVER      : Double,
                      VWAP          : Double,
                      SETTLE        : Double,
                      OPENINTEREST  : Double,
                      ASK1          : Double = 0.0,
                      ASK2          : Double = 0.0,
                      ASK3          : Double = 0.0,
                      ASK4          : Double = 0.0,
                      ASK5          : Double = 0.0,
                      BID1          : Double = 0.0,
                      BID2          : Double = 0.0,
                      BID3          : Double = 0.0,
                      BID4          : Double = 0.0,
                      BID5          : Double = 0.0,
                      ASKVOL1       : Double = 0.0,
                      ASKVOL2       : Double = 0.0,
                      ASKVOL3       : Double = 0.0,
                      ASKVOL4       : Double = 0.0,
                      ASKVOL5       : Double = 0.0,
                      BIDVOL1       : Double = 0.0,
                      BIDVOL2       : Double = 0.0,
                      BIDVOL3       : Double = 0.0,
                      BIDVOL4       : Double = 0.0,
                      BIDVOL5       : Double = 0.0)
    
    
    def toColumnSet(bars: Seq[Bar], fields_str: String, bar_view: Boolean) : ColumnSet = {
        
        val fields =
            if (fields_str != "") ("symbol,trade_date,time,freq" + fields_str).split(",").map (_.trim).filter( _ != "").toSet
            else null
        
        val SYMBOL        = new Array[String](bars.size)
        val CODE          = new Array[String](bars.size)
        val CYCLE         = new Array[String](bars.size)
        val DATE          = new Array[Int   ](bars.size)
        val TRADEDATE     = new Array[Int   ](bars.size)
        val TIME          = new Array[Int   ](bars.size)
        val OPEN          = new Array[Double](bars.size)
        val HIGH          = new Array[Double](bars.size)
        val LOW           = new Array[Double](bars.size)
        val CLOSE         = new Array[Double](bars.size)
        val VOLUME        = new Array[Double](bars.size)
        val TURNOVER      = new Array[Double](bars.size)
        val VWAP          = new Array[Double](bars.size)
        val SETTLE        = new Array[Double](bars.size)
        val OPENINTEREST  = new Array[Double](bars.size)
        val ASK1          = new Array[Double](bars.size)
        val ASK2          = new Array[Double](bars.size)
        val ASK3          = new Array[Double](bars.size)
        val ASK4          = new Array[Double](bars.size)
        val ASK5          = new Array[Double](bars.size)
        val BID1          = new Array[Double](bars.size)
        val BID2          = new Array[Double](bars.size)
        val BID3          = new Array[Double](bars.size)
        val BID4          = new Array[Double](bars.size)
        val BID5          = new Array[Double](bars.size)
        val ASKVOL1       = new Array[Double](bars.size)
        val ASKVOL2       = new Array[Double](bars.size)
        val ASKVOL3       = new Array[Double](bars.size)
        val ASKVOL4       = new Array[Double](bars.size)
        val ASKVOL5       = new Array[Double](bars.size)
        val BIDVOL1       = new Array[Double](bars.size)
        val BIDVOL2       = new Array[Double](bars.size)
        val BIDVOL3       = new Array[Double](bars.size)
        val BIDVOL4       = new Array[Double](bars.size)
        val BIDVOL5       = new Array[Double](bars.size)
        
        val time_div = 1
        
        for ( i <- bars.indices) {
            val bar = bars(i)
            SYMBOL       (i) = bar.SYMBOL
            CODE         (i) = bar.CODE
            CYCLE        (i) = bar.CYCLE
            DATE         (i) = bar.DATE
            TRADEDATE    (i) = bar.TRADEDATE
            TIME         (i) = bar.TIME/time_div
            OPEN         (i) = bar.OPEN
            HIGH         (i) = bar.HIGH
            LOW          (i) = bar.LOW
            CLOSE        (i) = bar.CLOSE
            VOLUME       (i) = bar.VOLUME
            TURNOVER     (i) = bar.TURNOVER
            VWAP         (i) = bar.VWAP
            SETTLE       (i) = bar.SETTLE
            OPENINTEREST (i) = bar.OPENINTEREST
            if(bar_view) {
                ASK1(i) = bar.ASK1
                ASK2(i) = bar.ASK2
                ASK3(i) = bar.ASK3
                ASK4(i) = bar.ASK4
                ASK5(i) = bar.ASK5
                BID1(i) = bar.BID1
                BID2(i) = bar.BID2
                BID3(i) = bar.BID3
                BID4(i) = bar.BID4
                BID5(i) = bar.BID5
                ASKVOL1(i) = bar.ASKVOL1
                ASKVOL2(i) = bar.ASKVOL2
                ASKVOL3(i) = bar.ASKVOL3
                ASKVOL4(i) = bar.ASKVOL4
                ASKVOL5(i) = bar.ASKVOL5
                BIDVOL1(i) = bar.BIDVOL1
                BIDVOL2(i) = bar.BIDVOL2
                BIDVOL3(i) = bar.BIDVOL3
                BIDVOL4(i) = bar.BIDVOL4
                BIDVOL5(i) = bar.BIDVOL5
            }
        }
        
        val map = mutable.HashMap[String, Array[_ <: Any ] ]()
        if ( fields == null || fields.contains( "symbol      ".trim() ) )     map += ("symbol      ".trim() -> SYMBOL        )
        if ( fields == null || fields.contains( "code        ".trim() ) )     map += ("code        ".trim() -> CODE          )
        if ( fields == null || fields.contains( "freq        ".trim() ) )     map += ("freq        ".trim() -> CYCLE         )
        if ( fields == null || fields.contains( "date        ".trim() ) )     map += ("date        ".trim() -> DATE          )
        if ( fields == null || fields.contains( "trade_date  ".trim() ) )     map += ("trade_date  ".trim() -> TRADEDATE     )
        if ( fields == null || fields.contains( "time        ".trim() ) )     map += ("time        ".trim() -> TIME          )
        if ( fields == null || fields.contains( "open        ".trim() ) )     map += ("open        ".trim() -> OPEN          )
        if ( fields == null || fields.contains( "high        ".trim() ) )     map += ("high        ".trim() -> HIGH          )
        if ( fields == null || fields.contains( "low         ".trim() ) )     map += ("low         ".trim() -> LOW           )
        if ( fields == null || fields.contains( "close       ".trim() ) )     map += ("close       ".trim() -> CLOSE         )
        if ( fields == null || fields.contains( "volume      ".trim() ) )     map += ("volume      ".trim() -> VOLUME        )
        if ( fields == null || fields.contains( "turnover    ".trim() ) )     map += ("turnover    ".trim() -> TURNOVER      )
        if ( fields == null || fields.contains( "vwap        ".trim() ) )     map += ("vwap        ".trim() -> VWAP          )
        if ( fields == null || fields.contains( "settle      ".trim() ) )     map += ("settle      ".trim() -> SETTLE          )
        if ( fields == null || fields.contains( "oi          ".trim() ) )     map += ("oi          ".trim() -> OPENINTEREST  )
        if(bar_view) {
            if (fields == null || fields.contains("askprice1   ".trim())) map += ("askprice1   ".trim() -> ASK1)
            if (fields == null || fields.contains("askprice2   ".trim())) map += ("askprice2   ".trim() -> ASK2)
            if (fields == null || fields.contains("askprice3   ".trim())) map += ("askprice3   ".trim() -> ASK3)
            if (fields == null || fields.contains("askprice4   ".trim())) map += ("askprice4   ".trim() -> ASK4)
            if (fields == null || fields.contains("askprice5   ".trim())) map += ("askprice5   ".trim() -> ASK5)
            if (fields == null || fields.contains("bidprice1   ".trim())) map += ("bidprice1   ".trim() -> BID1)
            if (fields == null || fields.contains("bidprice2   ".trim())) map += ("bidprice2   ".trim() -> BID2)
            if (fields == null || fields.contains("bidprice3   ".trim())) map += ("bidprice3   ".trim() -> BID3)
            if (fields == null || fields.contains("bidprice4   ".trim())) map += ("bidprice4   ".trim() -> BID4)
            if (fields == null || fields.contains("bidprice5   ".trim())) map += ("bidprice5   ".trim() -> BID5)
            if (fields == null || fields.contains("askvolume1  ".trim())) map += ("askvolume1  ".trim() -> ASKVOL1)
            if (fields == null || fields.contains("askvolume2  ".trim())) map += ("askvolume2  ".trim() -> ASKVOL2)
            if (fields == null || fields.contains("askvolume3  ".trim())) map += ("askvolume3  ".trim() -> ASKVOL3)
            if (fields == null || fields.contains("askvolume4  ".trim())) map += ("askvolume4  ".trim() -> ASKVOL4)
            if (fields == null || fields.contains("askvolume5  ".trim())) map += ("askvolume5  ".trim() -> ASKVOL5)
            if (fields == null || fields.contains("bidvolume1  ".trim())) map += ("bidvolume1  ".trim() -> BIDVOL1)
            if (fields == null || fields.contains("bidvolume2  ".trim())) map += ("bidvolume2  ".trim() -> BIDVOL2)
            if (fields == null || fields.contains("bidvolume3  ".trim())) map += ("bidvolume3  ".trim() -> BIDVOL3)
            if (fields == null || fields.contains("bidvolume4  ".trim())) map += ("bidvolume4  ".trim() -> BIDVOL4)
            if (fields == null || fields.contains("bidvolume5  ".trim())) map += ("bidvolume5  ".trim() -> BIDVOL5)
        }
        map.toMap
    }
}
