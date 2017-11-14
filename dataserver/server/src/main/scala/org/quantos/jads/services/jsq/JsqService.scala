
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

import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.duration.DurationInt
import akka.actor.{Actor, ActorRef}
import akka.actor.Props
import akka.pattern.ask
import akka.util.Timeout
import org.slf4j.LoggerFactory
import org.quantos.jads.Config
import org.quantos.jads.common.DataType.ColumnSet
import org.quantos.jads.gateway.JRpcServer.{JsonCallReq, JsonCallRsp}
import org.quantos.jads.services.jsq.QuoteSchema.IndicatorInfo
import org.quantos.utils.jrpc.JsonHelper

object JsqService {

    object InitReq

    lazy val actor = Config.akka.system.actorOf(Props[JsqService], "jsqService")

    def start() {
        actor ! InitReq
    }

    case class RemoveSubscriberActor(client: String)

    case class SubscribeReq (
                                symbol : String,
                                fields   : String
    )

    case class SubscribeRsp (
                                symbols : Array[String],
                                schema_id  : Int,
                                schema     : Array[IndicatorInfo],
                                sub_hash   : String

    )

    case class QueryQuotesReq (
                                  symbol : String,
                                  fields : String
                              )

    case class QueryQuotesRsp (
        value  : ColumnSet
    )

    case class HeartBeatReq(
        my_time  : Long
    )

    case class HeartBeatRsp(
        my_time   : Long,
        your_time : Long
    )

    case class QuoteInd (
        schema_id  : Int,
        indicators : Seq[Int],
        values     : Seq[Any]
    )

}


/**
 * JSQ Server
 *
 * TODO: remove client from subscriber if timeout
 *
 */
class JsqService extends Actor{

    import JsqService._

    val logger = LoggerFactory.getLogger("Server")

    var subscriberMap = Map[String, ActorRef] ()
    
    var publisherActor : ActorRef = _

    var queryActor: ActorRef = _
    

    def receive = {

        case InitReq => init()

        case JsonCallReq(client, method, params) => onJsonCallReq(client, method, params)
        
        case msg: RemoveSubscriberActor =>
            subscriberMap -= msg.client
            
        case x => println ("Unknown msg: " + x)        
    }
    

    def init() {

        logger.info("Start JSQ Service")

        publisherActor = context.system.actorOf(Props[PublisherActor], "jsq_publisherActor")
        
        queryActor = context.system.actorOf(Props[QueryActor], "jsq_queryActor")
    }

    def onJsonCallReq(client: String, method: String, params: Any) {

        try {
            method match {
                case "jsq.heartbeat"    => onHeartBeatReq(sender(), client, params)
                case "jsq.query"        => onQueryReq(sender(), client, params)
                case "jsq.subscribe"    => onSubscribeReq(sender(), client, params)
                case _  => sender() ! JsonCallRsp(null, message="unkown method: " + method)
            }
        } catch {
            case t: Throwable =>
                t.printStackTrace()
                sender() ! JsonCallRsp(null, message=t.getMessage)
        }
    }

    def onQueryReq(clientActor: ActorRef, client: String, params: Any) {

        val req = JsonHelper.convert[QueryQuotesReq](params)

        implicit val timeout = Timeout(5 seconds)

        val f = (this.queryActor ? req).mapTo[QueryQuotesRsp]

        f.onSuccess{ case x =>
            clientActor ! JsonCallRsp( x.value, null, 0)
        }

        f.onFailure { case e =>
            clientActor ! JsonCallRsp( null, e.getMessage, -1)
        }
    }
    
    def onHeartBeatReq(clientActor: ActorRef, client: String, params: Any) {
        val actor = this.subscriberMap.getOrElse(client, null)
        if (actor != null)
            actor ! SubscriberActor.KeepAlive()

        val req = JsonHelper.convert[HeartBeatReq](params)
        clientActor ! JsonCallRsp( HeartBeatRsp(System.currentTimeMillis, req.my_time), null, 0)
    }

    def onSubscribeReq(clientActor: ActorRef, client: String, params: Any) {

        val req = JsonHelper.convert[SubscribeReq](params)

        // TODO: use actorSelection for distribution computing
        var actor = this.subscriberMap.getOrElse(client, null)
        
        if (actor == null) {
            actor = this.context.system.actorOf(Props[SubscriberActor])
            actor ! SubscriberActor.InitReq(client, publisherActor)//, socket)
            subscriberMap += (client -> actor)
        }

        // subscribeActor will send response
        actor ! SubscriberActor.SubscribeReq(clientActor, client, req)
    }
    
}

