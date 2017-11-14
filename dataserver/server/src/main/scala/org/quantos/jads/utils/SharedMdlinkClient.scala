
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

import jzs.api.{MdlinkClient, QmsClient}
import jzs.msg.md.Md.{MarketDataInd, MarketQuote}

object SharedMdlinkClient {
    
    val mktQuoteMap = new java.util.concurrent.ConcurrentHashMap[String, MarketQuote]()

    var clientList = Array[ (MarketQuote)=>Unit ]()
    
    def quote(symbol: String) = {
        mktQuoteMap.get(symbol)
    }

    def onMarketQuote(quote: MarketQuote) {

        val symbol = BaseData.getSymbol(quote.getJzcode, "")

        val inst = BaseData.getInstrument(quote.getJzcode)
        if (inst == null) return

        val builder = MarketQuote.newBuilder(quote);
        builder.setSymbol(symbol);

        // 万得宏汇的指数行情中 TURNOVER单位为 百元。这里复原成元
        // TODO：应该在 mdlink 中修改！
        if (inst.insttype == 100 && (inst.market == 1 || inst.market == 2) ) {
            builder.setTurnover( quote.getTurnover * 100 )
        }

        val q = builder.build()
        mktQuoteMap.put(symbol, q);
        broadcast(q)
    }
    
    def registerClient( client : (MarketQuote)=>Unit ) {
        clientList +:= (client) 
    }

    def broadcast(q: MarketQuote) {
        clientList.foreach { x => x(q) }
    }

    
    def start(mdlinkAddr: String) {

        val mdlink = new MdlinkClient(mdlinkAddr,
            new MdlinkClient.Callback {

                def onMarketDataInd(ind: MarketDataInd) {
                    try {
                        val quote = (if (ind.hasFut()) ind.getFut else if (ind.hasStk) ind.getStk else null)
    
                        if (quote != null && quote.getJzcode != 0)
                            onMarketQuote(quote)
                    } catch {
                        case t: Throwable    => println("onMarketDataInd: " + t.getMessage) 
                    }
                }
            })

        mdlink.start()
    }

    def startMdlinkSim(qmsAddr: String) {

        println(" !!!!!!!!!!!!!!!!!!!! WARNING START MDLINK SIMULATION !!!!!!!!!!!!!!!!!!!!")

        new Thread(new Runnable() {
            def run() {
                val qms = new QmsClient
                qms.connect( qmsAddr )
                val quotes = new scala.collection.mutable.ArrayBuffer[MarketQuote]
                for (s <- BaseData.getAllSymbols()) {
                    val q = qms.getMarketQuote(s)
                    if (q != null)
                        quotes += q
                }

                var count = 0
                while (true) {

                    val now = (new java.util.Date().getTime / 1000) % (24 * 60 * 60)

                    for (i <- 0 until 100 if (count < quotes.size)) {
                        var quote = quotes(count)
                        onMarketQuote(quote)
                        count += 1
                    }

                    if (count >= quotes.size) count = 0
                    Thread.sleep(10)
                }
            }
        }).start()
    }

}

