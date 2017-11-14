
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

import scala.collection.JavaConversions._
import akka.actor.{Actor, ActorRef}
import org.quantos.jads.utils.{SharedMdlinkClient, TimeUtil}
import org.slf4j.LoggerFactory
import org.quantos.jads.Config
import org.quantos.jads.utils.TimeUtil._
import org.quantos.jads.utils.{BaseData, SharedMdlinkClient}
import jzs.msg.md.Md.{AskBid, QuoteStatic}

import scala.collection.mutable
import scala.collection.mutable.HashMap
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global
//import jzs.msg.md.Md.MarketDataInd
import jzs.msg.md.Md.MarketQuote

import jzs.api.QmsClient

/**
 * QueryActor
 * 
 *  接收全市场行情, 
 * 
 */

object QueryActor {

    object InitReq

    case class InitFromQmsReq (qmsQuotes: HashMap[Int, MarketQuote])
}


class QueryActor extends Actor {

    import QueryActor._
    import QuoteSchema._
    import QuoteSchema.IndicatorID._
    
    case class SpreadData (symbol: String, members: Seq[String], weights: Seq[Double])
    
    val logger = LoggerFactory.getLogger("QueryActor")

    var quoteMap = mutable.HashMap[Int, MarketQuote]()
    var symbolMap: Map[String, Int] = _

    self ! InitReq

    def receive = {
        case InitReq                          => init()
        case req : InitFromQmsReq             => initQuotes(req.qmsQuotes)
        case quote: MarketQuote                => onMarketQuote(quote)
        case req: JsqService.QueryQuotesReq    => onQueryQuotesReq(req)
        case x => println ("Unknown msg: " + x)
    }
    
    def init() {
        if(Config.conf.qms == null) {
            return
        }
        logger.info("Init Jsq.Query actor...")
        symbolMap =
           BaseData.getAllSymbols() map { case s =>
                s -> BaseData.getJzCode(s)
            } toMap
            
        quoteMap ++=
            symbolMap.map { case (_, jzcode) => jzcode -> null }
            
        SharedMdlinkClient.registerClient { x => self ! x }

        val qms = new QmsClient
        qms.connect(Config.conf.qms.addr)
        //symbolMap foreach { case (s,j) =>
        val f = Future  {
            try {
                val qmsQuotes = HashMap[Int, MarketQuote]()
                var count = 0
                for ((s, j) <- symbolMap) {
                    val q = qms.getMarketQuote(s)
                    if (q != null) {
                        qmsQuotes += (q.getJzcode -> q)
                        count += 1
                    }
                }
                qmsQuotes
            } catch {
                case t: Throwable => {
                    t.printStackTrace()
                    logger.error("Init from Qms error: " + t)
                }
            }
    
        }
    
        f onSuccess {
            case qmsQuotes: HashMap[Int, MarketQuote] => {
                self ! InitFromQmsReq(qmsQuotes)
            }
            case _ => {}
        }
        logger.info("Jsq.Query actor is ready")
    }
    
    def initQuotes(qmsQuotes: HashMap[Int, MarketQuote]) = {
        // init quote map with quotes from qms
        var count = 0
        qmsQuotes foreach { x =>
            if(quoteMap.getOrElse(x._1, null) == null) {
                quoteMap += (x._1 -> x._2)
                count += 1
            }
        }
        logger.info(s"Get ${count} quotes from Qms")
    }
    def onMarketQuote(quote: MarketQuote) {
        quoteMap += (quote.getJzcode -> quote)
    }

    val defaultIndicators = schema

    def onQueryQuotesReq(req: JsqService.QueryQuotesReq) {

        // Assume one actor has a unique path
        logger.info("onQueryQuotesReq: " + req.symbol)
        
        val codes = req.symbol.split(',').map { _.trim }.filter { _ != "" }

        val mkt_quotes =
            codes.map { case x =>
                val jzcode = symbolMap.getOrElse(x, 0)
                if (jzcode != 0)
                    x -> quoteMap.getOrElse(jzcode, null)
                else
                    x -> null
            } filter { _._2 != null }
        
        val quotes = mkt_quotes

        val indicators = {
            val subscribed = ("symbol,date,time," + req.fields).split(',').map { _.trim }.filter( _ != "")
                            . map { getIndicator(_) }.filter{ _ != null }.toSet
            if (subscribed.size != 3)
                subscribed.toArray
            else
                defaultIndicators
        }

        val ds = mutable.HashMap[String, Array[ _ <: Any]]()
        
        for ( ind <- indicators ) {
            ind.id match {
                case SYMBOL      => ds += (ind.name -> quotes.map { _._1                       })
                case DATE        => ds += (ind.name -> quotes.map { _._2.getQs.getDate         })
                case TRADE_DATE  => ds += (ind.name -> quotes.map { _._2.getQs.getTradeday     })
                case TIME        => ds += (ind.name -> quotes.map { _._2.getTime.toHMSMS.toInt }) // TODO: check format
                case OPEN        => ds += (ind.name -> quotes.map { _._2.getOpen               }) 
                case HIGH        => ds += (ind.name -> quotes.map { _._2.getHigh               })
                case LOW         => ds += (ind.name -> quotes.map { _._2.getLow                })
                case CLOSE       => ds += (ind.name -> quotes.map { _._2.getClose              })
                case LAST        => ds += (ind.name -> quotes.map { _._2.getLast               })
                case VOLUME      => ds += (ind.name -> quotes.map { _._2.getVolume             })
                case PRECLOSE    => ds += (ind.name -> quotes.map { _._2.getQs.getPreclose     })
                case PRESETTLE   => ds += (ind.name -> quotes.map { _._2.getQs.getPresettle    })
                case SETTLE      => ds += (ind.name -> quotes.map { _._2.getSettle             })
                case OPENINTEREST     => ds += (ind.name -> quotes.map { _._2.getInterest           })
                case PREOPENINTEREST  => ds += (ind.name -> quotes.map { _._2.getQs.getPreinterest  })
                case TURNOVER         => ds += (ind.name -> quotes.map { _._2.getTurnover           })
                case HIGHLIMIT        => ds += (ind.name -> quotes.map { _._2.getQs.getUplimit      })
                case LOWLIMIT         => ds += (ind.name -> quotes.map { _._2.getQs.getDownlimit    })
             //   case VWAP             => ds += (ind.name -> quotes.map { _._2.getVwap               })
                    
                case ASK1        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 1 ) v.getAb.getAskPrice(0) else 0 })
                case ASK2        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 2 ) v.getAb.getAskPrice(1) else 0 })
                case ASK3        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 3 ) v.getAb.getAskPrice(2) else 0 })
                case ASK4        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 4 ) v.getAb.getAskPrice(3) else 0 })
                case ASK5        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 5 ) v.getAb.getAskPrice(4) else 0 })
                case ASK6        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 6 ) v.getAb.getAskPrice(5) else 0 })
                case ASK7        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 7 ) v.getAb.getAskPrice(6) else 0 })
                case ASK8        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 8 ) v.getAb.getAskPrice(7) else 0 })
                case ASK9        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 9 ) v.getAb.getAskPrice(8) else 0 })
                case ASK10       => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskPriceCount >= 10) v.getAb.getAskPrice(9) else 0 })
                case BID1        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 1 ) v.getAb.getBidPrice(0) else 0 })
                case BID2        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 2 ) v.getAb.getBidPrice(1) else 0 })
                case BID3        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 3 ) v.getAb.getBidPrice(2) else 0 })
                case BID4        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 4 ) v.getAb.getBidPrice(3) else 0 })
                case BID5        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 5 ) v.getAb.getBidPrice(4) else 0 })
                case BID6        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 6 ) v.getAb.getBidPrice(5) else 0 })
                case BID7        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 7 ) v.getAb.getBidPrice(6) else 0 })
                case BID8        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 8 ) v.getAb.getBidPrice(7) else 0 })
                case BID9        => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 9 ) v.getAb.getBidPrice(8) else 0 })
                case BID10       => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidPriceCount >= 10) v.getAb.getBidPrice(9) else 0 })
                case ASKVOL1     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 1 ) v.getAb.getAskVolume(0) else 0 })
                case ASKVOL2     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 2 ) v.getAb.getAskVolume(1) else 0 })
                case ASKVOL3     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 3 ) v.getAb.getAskVolume(2) else 0 })
                case ASKVOL4     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 4 ) v.getAb.getAskVolume(3) else 0 })
                case ASKVOL5     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 5 ) v.getAb.getAskVolume(4) else 0 })
                case ASKVOL6     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 6 ) v.getAb.getAskVolume(5) else 0 })
                case ASKVOL7     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 7 ) v.getAb.getAskVolume(6) else 0 })
                case ASKVOL8     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 8 ) v.getAb.getAskVolume(7) else 0 })
                case ASKVOL9     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 9 ) v.getAb.getAskVolume(8) else 0 })
                case ASKVOL10    => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getAskVolumeCount >= 10) v.getAb.getAskVolume(9) else 0 })
                case BIDVOL1     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 1 ) v.getAb.getBidVolume(0) else 0 })
                case BIDVOL2     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 2 ) v.getAb.getBidVolume(1) else 0 })
                case BIDVOL3     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 3 ) v.getAb.getBidVolume(2) else 0 })
                case BIDVOL4     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 4 ) v.getAb.getBidVolume(3) else 0 })
                case BIDVOL5     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 5 ) v.getAb.getBidVolume(4) else 0 })
                case BIDVOL6     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 6 ) v.getAb.getBidVolume(5) else 0 })
                case BIDVOL7     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 7 ) v.getAb.getBidVolume(6) else 0 })
                case BIDVOL8     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 8 ) v.getAb.getBidVolume(7) else 0 })
                case BIDVOL9     => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 9 ) v.getAb.getBidVolume(8) else 0 })
                case BIDVOL10    => ds += (ind.name -> quotes.map { case (k,v) => if ( v.getAb.getBidVolumeCount >= 10) v.getAb.getBidVolume(9) else 0 })

                case _ =>
            }            
        }
        
        sender ! JsqService.QueryQuotesRsp(ds.toMap)
    }
}
