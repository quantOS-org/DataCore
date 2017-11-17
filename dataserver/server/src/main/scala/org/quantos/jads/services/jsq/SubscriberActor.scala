
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

import SubscriberActor._
import QuoteSchema._

import scala.concurrent.duration.{Duration,DurationInt}
import scala.collection.JavaConversions._
import scala.collection.mutable
import scala.language.experimental.macros
import akka.actor.{ActorRef, FSM}
import org.quantos.jads.utils.TimeUtil
import org.slf4j.LoggerFactory
import org.quantos.jads.utils.TimeUtil._
import org.quantos.jads.utils.BaseData
import org.quantos.jads.gateway.{JRpcServer, SessionActor}
import org.quantos.jads.gateway.JRpcServer.JsonCallRsp
import jzs.msg.md.Md.{AskBid, MarketQuote, QuoteStatic}

import scala.concurrent.Await
import akka.util.Timeout
import akka.pattern.ask

object SubscriberActor {

    case class InitReq (clientId: String, publisher: ActorRef)

    case class SubscribeReq(clientActor: ActorRef, client: String, req : JsqService.SubscribeReq)

    case class KeepAlive()

    val TIMER_KeepAliveTimeout = "KeepAliveTimeout"

    object Init
    object Subscribing
    object Active
    object Unsubscribing

}

class SubscriberActor extends FSM[Any, Any] {


    val logger = LoggerFactory.getLogger("SubscriberActor")

    var clientId : String = _

    var publisher:   ActorRef = _
    var jsq_server:  ActorRef = _

    var subscribedSybmols = Set[String]()
    var subscribedQuotes = mutable.HashMap[String, MarketQuote]()

    val defaultIndicators = schema.map { _.id } toSet
    var subscribedIndicators = defaultIndicators

    val sf_quotes = mutable.HashMap[Int, MarketQuote] ()

    startWith(Init, null)

    when(Init) {
        case Event(req: InitReq, _) =>

            clientId  = req.clientId
            publisher = req.publisher
            jsq_server    = sender
            logger.info(s"new subscriber actor for $clientId")

            
            implicit val timeout = Timeout(65 seconds)
            val f = (JRpcServer.session_actor ? SessionActor.SubscribeSessionTerminatedIndReq(clientId))
                            .mapTo[SessionActor.SubscribeSessionTerminatedIndRsp]
    
            val r = Await.result(f, Duration.Inf)
            goto ( if (r.success) Active else Unsubscribing)
    }

    when(Active) {
        case Event(req: SubscribeReq, _) =>

            logger.info("subscribe: " + req.req.symbol + ", " + req.req.fields)

            if ( processSubscribeReq(req) )
                goto(Subscribing) using (req)
            else
                stay

        case Event(ind: PublisherActor.QuoteInd, _) =>
            try {
                subscribedQuotes += ind.symbol->ind.quote
                if (this.subscribedSybmols != null && this.subscribedSybmols.contains(ind.symbol))
                    publish(ind.symbol, ind.quote)

                stay
            } catch {
                case t: Throwable =>
                    logger.error("SubscriberActor:" + t.getClass + ", " + t.getMessage)
                    goto(Unsubscribing)
            }
    }

    when(Subscribing, stateTimeout = 3 second) {
        case Event(rsp: PublisherActor.QueryQuoteRsp,  _) =>
            onQueryQuotesRsp(rsp)
            stay

        case Event(rsp: PublisherActor.SubscribeRsp,  req: SubscribeReq) =>

            val sub_symbols =  rsp.subscribed.toSet
            subscribedSybmols ++= sub_symbols
//            var sub_spds = new mutable.ListBuffer[String]()
//
//            subscribedSpread.values foreach { x=>
//                var all_subscribed = true
//                for (member <- x.members) {
//                    if(!sub_symbols.contains(member)) {
//                        all_subscribed = false
//                    }
//                }
//
//                if(all_subscribed) {
//                    for(member <- x.members) {
//                        var spreads = instcode2spreads.getOrElse(member, null)
//                        if(spreads == null) {
//                            spreads = new mutable.ListBuffer[SpreadData]()
//                            instcode2spreads += member->spreads
//                        }
//                        if(!spreads.contains(x)) {
//                            spreads.append(x)
//                        }
//                    }
//                    sub_spds.append(x.symbol)
//                }
//            }

//            if (subscribedSybmols.nonEmpty || sub_spds.nonEmpty) {
//                req.clientActor ! JsonCallRsp()
//            }

            val hash_code = (subscribedSybmols).toSeq
                            .sorted.mkString(",").hashCode.toString

            val session = SessionActor.session_mgr.getSession(clientId)
            if (session!=null)
                session.subscription_hash = hash_code
            else {
                logger.error("can't find session: " + clientId)
            }

            val out_rsp = JsqService.SubscribeRsp(
                    symbols = (subscribedSybmols).toArray.sorted,
                    schema_id = schemaId,
                    schema = schema,
                    sub_hash = hash_code
                )

            req.clientActor ! JsonCallRsp(out_rsp, null, 0)

            goto(Active)

        case Event( StateTimeout, _) => goto(Unsubscribing)
    }
    
    onTransition {
        case _ -> Unsubscribing => publisher ! PublisherActor.UnSubscribeReq()
    }
    
    when(Unsubscribing) {
        case Event( _: PublisherActor.UnSubscribeRsp, _) =>
            logger.info("unsubscribe")
            jsq_server ! JsqService.RemoveSubscriberActor(clientId)
            stop
    }
    
    whenUnhandled {
//        case Event(_: KeepAlive, _) =>
//            setTimer(TIMER_KeepAliveTimeout, TIMER_KeepAliveTimeout, 1 minutes, true)
//            logger.info("keep alive")
//            stay
        
        case Event(SessionActor.SessionTerminatedInd, _) if stateName != Unsubscribing =>
            logger.info("unhandled event, goto unsubscribe")
            goto (Unsubscribing)
        
        // Drop all quote if it is not int active
        case Event(ind: PublisherActor.QuoteInd, _) => stay

        case Event(x, _) =>
            logger.info("Unknown Event " + x + "," + "on" + stateName)
            stay
    }
    def onQueryQuotesRsp(rsp: PublisherActor.QueryQuoteRsp) = {
        for(quote <- rsp.quotes) {
            val instcode = BaseData.getSymbol(quote.getJzcode)
            subscribedQuotes += instcode->quote
        }
    }

    /**
      * Extract symbols from request and
      * 1. add normal symbols to subscribedSybmols
      *
      *
      * @param req
      * @return subscribedSybmols
      */
    def extractSymbols(req: SubscribeReq): Seq[String] = {

        val security   = req.req.symbol.split("[;]") map { _.trim } filter ( _ != "" )
        var symbols     = mutable.ListBuffer[String]()
        var sub_symbols = mutable.ListBuffer[String]()

        security foreach { x =>
            val mkt_symbols = x.split(",") map { _.trim } filter ( _ != "" )
            symbols = symbols.union(mkt_symbols)
            sub_symbols = sub_symbols.union(mkt_symbols).distinct
        }

        this.subscribedSybmols = sub_symbols.toSet
        symbols.toSeq
    }

    def processSubscribeReq(req: SubscribeReq): Boolean = {
        val symbols = extractSymbols(req)

        clientId = req.client

        if (req.req.fields != null && req.req.fields.nonEmpty) {
            subscribedIndicators =
                req.req.fields.split(",").map{ f =>
                    val ind = schemaMap.getOrElse(f.trim, null)
                    if (ind != null)
                        ind.id
                    else
                        -1
                } filter { _ != -1 } toSet

            subscribedIndicators += IndicatorID.SYMBOL
            subscribedIndicators += IndicatorID.DATE
            subscribedIndicators += IndicatorID.TIME

        } else
            subscribedIndicators = defaultIndicators

        if (subscribedIndicators.size == 3) {

            req.clientActor ! JsonCallRsp( null, "wrong fields", -1)

            false

        } else {


            logger.info("processSubscribeReq: " + symbols.mkString("Symbol(",",", ")"))

            val jzcodes = symbols map {x => BaseData.getJzCode(x)} toSeq

            publisher ! PublisherActor.QueryQuotesReq(symbols )

            publisher ! PublisherActor.SubscribeReq(symbols )

            true
        }
    }


    


    def publish(symbol: String, quote: MarketQuote) = {

        //val ind = Jsq.QuoteInd.newBuilder().setSchemaId( schemaId )

        import IndicatorID._

        // TODO: publish only if any subscribed indicator has been changed

        //val list = new InicatorList
        //val q = mutable.HashMap[Int, Any]() // Use String -> Any instead?

        val indicators = mutable.ArrayBuffer[Int]()
        val values     = mutable.ArrayBuffer[Any]()

        this.subscribedIndicators foreach {
            case SYMBOL          => indicators +=  SYMBOL          ; values +=  symbol
            case DATE            => indicators +=  DATE            ; values +=  quote.getQs.getDate
            case TRADE_DATE      => indicators +=  TRADE_DATE      ; values +=  quote.getQs.getTradeday
            case TIME            => indicators +=  TIME            ; values +=  quote.getTime.toHMSMS.toInt    // TODO: check format
            case OPEN            => indicators +=  OPEN            ; values +=  quote.getOpen
            case HIGH            => indicators +=  HIGH            ; values +=  quote.getHigh
            case LOW             => indicators +=  LOW             ; values +=  quote.getLow
            case CLOSE           => indicators +=  CLOSE           ; values +=  quote.getClose
            case LAST            => indicators +=  LAST            ; values +=  quote.getLast
            case VWAP            => indicators +=  VWAP            ; values +=  quote.getVwap
            case VOLUME          => indicators +=  VOLUME          ; values +=  quote.getVolume
            case TURNOVER        => indicators +=  TURNOVER        ; values +=  quote.getTurnover
            case PRECLOSE        => indicators +=  PRECLOSE        ; values +=  quote.getQs.getPreclose
            case PRESETTLE       => indicators +=  PRESETTLE       ; values +=  quote.getQs.getPresettle
            case SETTLE          => indicators +=  SETTLE          ; values +=  quote.getSettle
            case OPENINTEREST    => indicators +=  OPENINTEREST    ; values +=  quote.getInterest
            case PREOPENINTEREST => indicators +=  PREOPENINTEREST ; values +=  quote.getQs.getPreinterest
            case HIGHLIMIT       => indicators +=  HIGHLIMIT       ; values +=  quote.getQs.getUplimit
            case LOWLIMIT        => indicators +=  LOWLIMIT        ; values +=  quote.getQs.getDownlimit
            

            case ASK1  if quote.getAb.getAskPriceCount >= 1   => indicators +=  ASK1  ; values += quote.getAb.getAskPrice(0)
            case ASK2  if quote.getAb.getAskPriceCount >= 2   => indicators +=  ASK2  ; values += quote.getAb.getAskPrice(1)
            case ASK3  if quote.getAb.getAskPriceCount >= 3   => indicators +=  ASK3  ; values += quote.getAb.getAskPrice(2)
            case ASK4  if quote.getAb.getAskPriceCount >= 4   => indicators +=  ASK4  ; values += quote.getAb.getAskPrice(3)
            case ASK5  if quote.getAb.getAskPriceCount >= 5   => indicators +=  ASK5  ; values += quote.getAb.getAskPrice(4)
            case BID1  if quote.getAb.getBidPriceCount >= 1   => indicators +=  BID1  ; values += quote.getAb.getBidPrice(0)
            case BID2  if quote.getAb.getBidPriceCount >= 2   => indicators +=  BID2  ; values += quote.getAb.getBidPrice(1)
            case BID3  if quote.getAb.getBidPriceCount >= 3   => indicators +=  BID3  ; values += quote.getAb.getBidPrice(2)
            case BID4  if quote.getAb.getBidPriceCount >= 4   => indicators +=  BID4  ; values += quote.getAb.getBidPrice(3)
            case BID5  if quote.getAb.getBidPriceCount >= 5   => indicators +=  BID5  ; values += quote.getAb.getBidPrice(4)

            case ASK6  if quote.getAb.getAskPriceCount >= 6   => indicators +=  ASK6  ; values += quote.getAb.getAskPrice(5)
            case ASK7  if quote.getAb.getAskPriceCount >= 7   => indicators +=  ASK7  ; values += quote.getAb.getAskPrice(6)
            case ASK8  if quote.getAb.getAskPriceCount >= 8   => indicators +=  ASK8  ; values += quote.getAb.getAskPrice(7)
            case ASK9  if quote.getAb.getAskPriceCount >= 9   => indicators +=  ASK9  ; values += quote.getAb.getAskPrice(8)
            case ASK10 if quote.getAb.getAskPriceCount >= 10  => indicators +=  ASK10 ; values += quote.getAb.getAskPrice(9)
            case BID6  if quote.getAb.getBidPriceCount >= 6   => indicators +=  BID6  ; values += quote.getAb.getBidPrice(5)
            case BID7  if quote.getAb.getBidPriceCount >= 7   => indicators +=  BID7  ; values += quote.getAb.getBidPrice(6)
            case BID8  if quote.getAb.getBidPriceCount >= 8   => indicators +=  BID8  ; values += quote.getAb.getBidPrice(7)
            case BID9  if quote.getAb.getBidPriceCount >= 9   => indicators +=  BID9  ; values += quote.getAb.getBidPrice(8)
            case BID10 if quote.getAb.getBidPriceCount >= 10  => indicators +=  BID10 ; values += quote.getAb.getBidPrice(9)

            case ASKVOL1  if quote.getAb.getAskVolumeCount >= 1   => indicators +=  ASKVOL1  ; values += quote.getAb.getAskVolume(0)
            case ASKVOL2  if quote.getAb.getAskVolumeCount >= 2   => indicators +=  ASKVOL2  ; values += quote.getAb.getAskVolume(1)
            case ASKVOL3  if quote.getAb.getAskVolumeCount >= 3   => indicators +=  ASKVOL3  ; values += quote.getAb.getAskVolume(2)
            case ASKVOL4  if quote.getAb.getAskVolumeCount >= 4   => indicators +=  ASKVOL4  ; values += quote.getAb.getAskVolume(3)
            case ASKVOL5  if quote.getAb.getAskVolumeCount >= 5   => indicators +=  ASKVOL5  ; values += quote.getAb.getAskVolume(4)
            case BIDVOL1  if quote.getAb.getBidVolumeCount >= 1   => indicators +=  BIDVOL1  ; values += quote.getAb.getBidVolume(0)
            case BIDVOL2  if quote.getAb.getBidVolumeCount >= 2   => indicators +=  BIDVOL2  ; values += quote.getAb.getBidVolume(1)
            case BIDVOL3  if quote.getAb.getBidVolumeCount >= 3   => indicators +=  BIDVOL3  ; values += quote.getAb.getBidVolume(2)
            case BIDVOL4  if quote.getAb.getBidVolumeCount >= 4   => indicators +=  BIDVOL4  ; values += quote.getAb.getBidVolume(3)
            case BIDVOL5  if quote.getAb.getBidVolumeCount >= 5   => indicators +=  BIDVOL5  ; values += quote.getAb.getBidVolume(4)

            case ASKVOL6  if quote.getAb.getAskVolumeCount >= 6   => indicators +=  ASKVOL6  ; values += quote.getAb.getAskVolume(5)
            case ASKVOL7  if quote.getAb.getAskVolumeCount >= 7   => indicators +=  ASKVOL7  ; values += quote.getAb.getAskVolume(6)
            case ASKVOL8  if quote.getAb.getAskVolumeCount >= 8   => indicators +=  ASKVOL8  ; values += quote.getAb.getAskVolume(7)
            case ASKVOL9  if quote.getAb.getAskVolumeCount >= 9   => indicators +=  ASKVOL9  ; values += quote.getAb.getAskVolume(8)
            case ASKVOL10 if quote.getAb.getAskVolumeCount >= 10  => indicators +=  ASKVOL10 ; values += quote.getAb.getAskVolume(9)
            case BIDVOL6  if quote.getAb.getBidVolumeCount >= 6   => indicators +=  BIDVOL6  ; values += quote.getAb.getBidVolume(5)
            case BIDVOL7  if quote.getAb.getBidVolumeCount >= 7   => indicators +=  BIDVOL7  ; values += quote.getAb.getBidVolume(6)
            case BIDVOL8  if quote.getAb.getBidVolumeCount >= 8   => indicators +=  BIDVOL8  ; values += quote.getAb.getBidVolume(7)
            case BIDVOL9  if quote.getAb.getBidVolumeCount >= 9   => indicators +=  BIDVOL9  ; values += quote.getAb.getBidVolume(8)
            case BIDVOL10 if quote.getAb.getBidVolumeCount >= 10  => indicators +=  BIDVOL10 ; values += quote.getAb.getBidVolume(9)
            case _ =>
        }

        val ind = JsqService.QuoteInd(schemaId, indicators, values)

        JRpcServer.rpcserver.notify(clientId, "jsq.quote_ind", ind)
    }

    initialize()
}
