
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

package org.quantos.jads.services.jsq

object QuoteSchema {

    object IndicatorID {
        val SYMBOL = 1
        val DATE = 2
        val TIME = 3
        val OPEN = 4
        val HIGH = 5
        val LOW = 6
        val CLOSE = 7
        val LAST = 8
        val VOLUME = 9
        val ASK1 = 10
        val ASK2 = 11
        val ASK3 = 12
        val ASK4 = 13
        val ASK5 = 14
        val ASK6 = 15
        val ASK7 = 16
        val ASK8 = 17
        val ASK9 = 18
        val ASK10 = 19
        val BID1 = 20
        val BID2 = 21
        val BID3 = 22
        val BID4 = 23
        val BID5 = 24
        val BID6 = 25
        val BID7 = 26
        val BID8 = 27
        val BID9 = 28
        val BID10 = 29
        val MID = 30
        val ASKVOL1 = 31
        val ASKVOL2 = 32
        val ASKVOL3 = 33
        val ASKVOL4 = 34
        val ASKVOL5 = 35
        val ASKVOL6 = 36
        val ASKVOL7 = 37
        val ASKVOL8 = 38
        val ASKVOL9 = 39
        val ASKVOL10 = 40
        val BIDVOL1 = 41
        val BIDVOL2 = 42
        val BIDVOL3 = 43
        val BIDVOL4 = 44
        val BIDVOL5 = 45
        val BIDVOL6 = 46
        val BIDVOL7 = 47
        val BIDVOL8 = 48
        val BIDVOL9 = 49
        val BIDVOL10 = 50
        val PRECLOSE      = 51
        val PRESETTLE     = 52
        val SETTLE        = 53
        val OPENINTEREST  = 54
        val PREOPENINTEREST  = 55
        val TURNOVER         = 56
        val HIGHLIMIT        = 57
        val LOWLIMIT         = 58
        val LAST_PREMIUM     = 59
        val ASK1_PREMIUM     = 60
        val BID1_PREMIUM     = 61
        val ACTIVEDATE     = 62
        val TRADE_DATE     = 63
        val VWAP           = 64
    }

    case class IndicatorInfo(name: String, id: Int, data_type: String)

    import IndicatorID._

    val schema = Array(
        IndicatorInfo("symbol",             SYMBOL,             "string"),
        IndicatorInfo("date",               DATE,               "int"),
        IndicatorInfo("trade_date",         TRADE_DATE,         "int"),
        IndicatorInfo("time",               TIME,               "int"),
        IndicatorInfo("open",               OPEN,               "double"),
        IndicatorInfo("high",               HIGH,               "double"),
        IndicatorInfo("low",                LOW,                "double"),
        IndicatorInfo("close",              CLOSE,              "double"),
        IndicatorInfo("last",               LAST,               "double"),
        IndicatorInfo("volume",             VOLUME,             "double"),
        IndicatorInfo("askprice1",          ASK1,               "double"),
        IndicatorInfo("askprice2",          ASK2,               "double"),
        IndicatorInfo("askprice3",          ASK3,               "double"),
        IndicatorInfo("askprice4",          ASK4,               "double"),
        IndicatorInfo("askprice5",          ASK5,               "double"),
        IndicatorInfo("askprice6",          ASK6,               "double"),
        IndicatorInfo("askprice7",          ASK7,               "double"),
        IndicatorInfo("askprice8",          ASK8,               "double"),
        IndicatorInfo("askprice9",          ASK9,               "double"),
        IndicatorInfo("askprice10",         ASK10,              "double"),
        IndicatorInfo("bidprice1",          BID1,               "double"),
        IndicatorInfo("bidprice2",          BID2,               "double"),
        IndicatorInfo("bidprice3",          BID3,               "double"),
        IndicatorInfo("bidprice4",          BID4,               "double"),
        IndicatorInfo("bidprice5",          BID5,               "double"),
        IndicatorInfo("bidprice6",          BID6,               "double"),
        IndicatorInfo("bidprice7",          BID7,               "double"),
        IndicatorInfo("bidprice8",          BID8,               "double"),
        IndicatorInfo("bidprice9",          BID9,               "double"),
        IndicatorInfo("bidprice10",         BID10,              "double"),
        IndicatorInfo("mid",                MID,                "double"),
        IndicatorInfo("askvolume1",         ASKVOL1,            "double"),
        IndicatorInfo("askvolume2",         ASKVOL2,            "double"),
        IndicatorInfo("askvolume3",         ASKVOL3,            "double"),
        IndicatorInfo("askvolume4",         ASKVOL4,            "double"),
        IndicatorInfo("askvolume5",         ASKVOL5,            "double"),
        IndicatorInfo("askvolume6",         ASKVOL6,            "double"),
        IndicatorInfo("askvolume7",         ASKVOL7,            "double"),
        IndicatorInfo("askvolume8",         ASKVOL8,            "double"),
        IndicatorInfo("askvolume9",         ASKVOL9,            "double"),
        IndicatorInfo("askvolume10",        ASKVOL10,           "double"),
        IndicatorInfo("bidvolume1",         BIDVOL1,            "double"),
        IndicatorInfo("bidvolume2",         BIDVOL2,            "double"),
        IndicatorInfo("bidvolume3",         BIDVOL3,            "double"),
        IndicatorInfo("bidvolume4",         BIDVOL4,            "double"),
        IndicatorInfo("bidvolume5",         BIDVOL5,            "double"),
        IndicatorInfo("bidvolume6",         BIDVOL6,            "double"),
        IndicatorInfo("bidvolume7",         BIDVOL7,            "double"),
        IndicatorInfo("bidvolume8",         BIDVOL8,            "double"),
        IndicatorInfo("bidvolume9",         BIDVOL9,            "double"),
        IndicatorInfo("bidvolume10",        BIDVOL10,           "double"),
        IndicatorInfo("preclose",           PRECLOSE,           "double"),
        IndicatorInfo("presettle",          PRESETTLE,          "double"),
        IndicatorInfo("settle",             SETTLE,             "double"),
        IndicatorInfo("oi",                 OPENINTEREST,       "int64"),
        IndicatorInfo("preoi",              PREOPENINTEREST,    "int64"),
        IndicatorInfo("turnover",           TURNOVER,           "double"),
        IndicatorInfo("limit_up",           HIGHLIMIT,          "double"),
        IndicatorInfo("limit_down",         LOWLIMIT,           "double"),
        IndicatorInfo("last_premium",       LAST_PREMIUM,       "double"),
        IndicatorInfo("ask1_premium",       ASK1_PREMIUM,       "double"),
        IndicatorInfo("bid1_premium",       BID1_PREMIUM,       "double"),
        IndicatorInfo("activedate",         ACTIVEDATE,         "int"),
        IndicatorInfo("vwap",               VWAP,               "double")
        )

    // unique ID for quote schema
    val schemaId = {
        val tmp = schema map { x => x.name + "_" + x.id.toString + "_" + x.data_type }
        tmp.mkString(",").hashCode
    }

    val schemaMap: Map[String, IndicatorInfo] = {
        schema map { x => x.name -> x } toMap
    }

    def _wrongIndicatorInfo = IndicatorInfo("WRONG", 0, "")

    def getIndicatorID(name: String) = schemaMap.getOrElse(name, _wrongIndicatorInfo).id

    def getIndicator(name: String) = schemaMap.getOrElse(name, null)

}
