
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
import scala.collection.mutable.ArrayBuffer
import akka.actor.{Actor, ActorRef}
import org.quantos.jads.utils.SharedMdlinkClient
import org.slf4j.LoggerFactory
import org.quantos.jads.utils.BaseData
import jzs.msg.md.Md.MarketQuote


/**
 * PublihserActor
 *  接收全市场行情, 保持最新的快照, 发布最新的行情给 SubscriberActor
 *
 */
object PublisherActor {

    object InitReq

    // 订阅新的代码, 删除以前订阅的
    case class SubscribeReq(symbols: Seq[String] )

    // 返回订阅成功的代码
    case class SubscribeRsp(subscribed: Seq[String] )
    case class UnSubscribeReq()

    case class UnSubscribeRsp()
    case class QueryQuotesReq(codes: Seq[String])
    case class QueryQuoteRsp( quotes: Seq[MarketQuote])

    case class QuoteInd( symbol: String, quote: MarketQuote )

}

class PublisherActor extends Actor {

    import PublisherActor._

    val logger = LoggerFactory.getLogger("PublisherActor")

    case class QuoteSub( symbol: String ) {
            var quote: MarketQuote = _
            val subscriberList = ArrayBuffer[ActorRef]()
    }

    var quoteSubMap: Map[Int, QuoteSub] = _

    var subscriberInfoMap = Map[String, Seq[String] ]()

    self ! InitReq

    def receive = {

        case InitReq => init()

        case quote: MarketQuote     => onMarketQuote(quote)
        case req: SubscribeReq      => onSubscribeReq( req, sender)
        case req: UnSubscribeReq    => onUnSubscribeReq( req, sender)
        case req: QueryQuotesReq    => onMarketQuoteReq(req, sender)
        case x => println ("Unknown msg: " + x)
    }

    def init() {
        SharedMdlinkClient.registerClient { x => self ! x }

        quoteSubMap =
            BaseData.getAllSymbols() map { case s =>
                BaseData.getJzCode(s) -> QuoteSub(s)
            } toMap
    }
    
    def onMarketQuoteReq(req: QueryQuotesReq, client: ActorRef)= {
        val quotes = req.codes map { x =>
                        val jzcode = BaseData.getJzCode(x)
                        val quoteSub = quoteSubMap.getOrElse(jzcode, null)
                        if(quoteSub != null) {
                            quoteSub.quote
                        }
                        else null
                    } filter( _ != null )
        
        client ! QueryQuoteRsp(quotes)
    }
    
    def onMarketQuote(quote: MarketQuote) {

        val sub = quoteSubMap.getOrElse(quote.getJzcode, null)
        if (sub == null) return
        sub.quote = quote
        val ind = QuoteInd(sub.symbol, quote)
        // TODO: check if it is ok when actor is dead
        sub.subscriberList foreach { _ ! ind }
    }

    def onSubscribeReq(req: SubscribeReq, client: ActorRef) {

        // Assume one actor has a unique path
        logger.info("onSubscribeReq: " + req.symbols.mkString(","))
        val id = client.path.toString

        var oldList = subscriberInfoMap.getOrElse(id, null)

        if (oldList == null) oldList = Seq[String]()
        val newList = req.symbols

        val exist_subscribed = oldList.intersect(req.symbols)

        for ( s <- oldList diff newList ) {

            val sub = quoteSubMap.getOrElse(BaseData.getJzCode(s), null)
            if (sub != null)
                sub.subscriberList.dropWhile { x => x.path.toString == id }
        }

        val new_subscribed = ArrayBuffer[String]()

        for ( s <- req.symbols diff exist_subscribed ) {
            val sub = quoteSubMap.getOrElse(BaseData.getJzCode(s), null)
            if (sub != null) {
                sub.subscriberList.add(client)
                new_subscribed += s
            }
        }

        val subscribed = exist_subscribed ++ new_subscribed
        logger.info("subscribed: " + ( exist_subscribed, new_subscribed, subscribed) )

        subscriberInfoMap += ( id -> subscribed )

        client ! SubscribeRsp(subscribed)
    }

    def onUnSubscribeReq(req: UnSubscribeReq, client: ActorRef) {

        // Assume one actor has a unique path
        val id = client.path.toString

        var oldList = subscriberInfoMap.getOrElse(id, null)
        if (oldList == null) oldList = Seq[String]()

        for ( s <- oldList ) {

            val sub = quoteSubMap.getOrElse(BaseData.getJzCode(s), null)
            if (sub != null)
                sub.subscriberList.dropWhile { x => x.path.toString == id }
        }

        subscriberInfoMap -= id
        client ! UnSubscribeRsp()
    }

}
