
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

package org.quantos.jads.gateway

import java.util.concurrent.ConcurrentHashMap

import akka.actor.{Actor, ActorRef}
import com.fasterxml.jackson.annotation.JsonIgnoreProperties
import org.quantos.jads.Config
import org.quantos.jads.utils.CommonRequestHandler
import org.quantos.utils.jrpc.JsonHelper

import scala.collection.JavaConverters._
import scala.collection.mutable
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.duration.{DurationInt, _}

object SessionActor {

    case class LoginReq(
            username:    String = "",
            password:    String = "")

    case class UserInfo (
            username:           String,
            name:               String)

    case class LoginRsp (
            user_info:   UserInfo = null,
            err_msg:     String   = "")


    case class UseStrategyReq (
        account_id: Int
    )

    case class SubStrategiesReq (
        account_ids: List[Int]
    )
    
    case class SessionTerminatedInd ( client_id: String  )
    case class SubscribeSessionTerminatedIndReq (client_id : String)
    case class SubscribeSessionTerminatedIndRsp (client_id : String, success : Boolean)

    val session_mgr = new SessionManager

}

case class Session (
        username: String,
        name:     String,
        session:  String,
        client:   String,
        var subscription_hash : String = "",
        val query_records : mutable.Queue[Long] = mutable.Queue[Long] ()
    )

@JsonIgnoreProperties
case class UserInfoData( user_id: Int,
                   username: String,
                   role:String,
                   phone:String,
                   email:String )

@JsonIgnoreProperties
case class UserCheckRsp( code: Int,
                         msg: String,
                         data: UserInfoData
                       )

class SessionManager {
    val logger = org.slf4j.LoggerFactory.getLogger(getClass.getSimpleName)
    val flow_control = if (Config.conf.flow_control<= 0) 50 else Config.conf.flow_control
    
    private case class Item(deadTime: Long, data: Session)

    private val clientSessionMap = new ConcurrentHashMap[String, Item]().asScala // client -> session
    private val accountClientMap = new ConcurrentHashMap[Int, mutable.HashSet[String]]().asScala // account -> session

    private def put(id: String, data: Session, timeout: Duration = 1 minutes)  {
        if (timeout != Duration.Inf )
            clientSessionMap +=  id ->  Item( System.currentTimeMillis() + timeout.toMillis,  data)
        else
            clientSessionMap += id ->  Item( System.currentTimeMillis() + (100 days).toMillis,  data)
    }

    private def get(id: String, defaultValue: Session = null): Session = {
        val item = clientSessionMap.getOrElse(id, null)

        try {
            if (item != null )
                item.data.asInstanceOf[Session]
            else
                defaultValue
        } catch {
            case _ : Throwable => defaultValue
        }
    }

    // return removed session
    def check() : Seq[String] = {
        val now = System.currentTimeMillis()
        val removed = mutable.ArrayBuffer[String]()
        clientSessionMap.filter( _._2.deadTime < now ) foreach { x =>
            logger.info(s"cachemgr: remove dead item, client id: ${x._1}, session: ${x._2}")
            removed += x._1
            clientSessionMap -= x._1
        }
        removed
    }
    
    def checkFlowControl(session: Session) : String = {
        val now = System.currentTimeMillis()
        if(session.query_records.size < flow_control) {
            session.query_records += now
            ""
        } else {
            val front_time = session.query_records.front
            while (session.query_records.size >= flow_control) {
                session.query_records.dequeue()
            }
            session.query_records += now
            if (now - front_time < (1 seconds).toMillis) {
                return "Query too frequently"
            } else {
                return ""
            }
        }
    }

    def getSession(client: String) : Session = {
        get(client, null)
    }
    
    def checkUser(username: String, password: String) : Boolean = {
        return true
    }
    
    def createSession(client: String, username: String, password: String) : Session= {

        // FIXME:
        if (checkUser(username, password)) {
            val token = System.currentTimeMillis.toString
            val session = Session(username, "NAME: FIXME", token, client)
            put(client, session, 60 seconds)
            session
        } else {
            null
        }
    }

    def destroySession(client: String) {
        val session = getSession(client)
        if (session != null) {
            clientSessionMap -= client
            //_getClients(session.cur_account) -= client
        }
    }

    def refreshSession(client: String) : Session = {
        val session = get(client, null)
        if (session != null ) {
            put(client, session, 60 seconds)
            session
        } else {
            null
        }
    }
}

class SessionActor extends Actor with CommonRequestHandler {

    import JRpcServer._
    import SessionActor._


    val logger = org.slf4j.LoggerFactory.getLogger("SessionActor")

    val methodMap = Map [ String, (ActorRef, String, Any) => Unit ] (

            ".sys.heartbeat"            -> sys_heartbeat _,

             "auth.login"               -> auth_login _ ,
             "auth.logout"              -> auth_logout _
        )

    val CHECK_SESSION_TIMER = "Timer_CheckSessionTimer"

    this.context.system.scheduler.schedule(1 seconds, 1 seconds, self, CHECK_SESSION_TIMER)
    
    val subscriber_map = mutable.HashMap[String, mutable.ArrayBuffer[ActorRef]]()

    override def receive = {
        // rpc request
        case JsonCallReq(client, method, params) => onJsonCallReq(client, method, params)

        case CHECK_SESSION_TIMER      => onCheckSession()

        case SubscribeSessionTerminatedIndReq(client) => onSubscribeSessionTerminatedIndReq(client)
    }

    def onJsonCallReq(client: String, method: String, params: Any) {
        try {
            val handler = methodMap.getOrElse(method, null)
            if (handler != null )
                handler(sender, client, params)
            else
                sender() ! JsonCallRsp(null, message="UNKOWN METHOD: " + method)
        } catch {
            case t: Throwable =>
                t.printStackTrace()
                sender() ! JsonCallRsp(null, message=t.getMessage)
        }
    }

    /*
     * Client should call .sys.heartbeat every seconds in order to keep trade session
     */
    def sys_heartbeat(clientActor: ActorRef, client: String, params: Any) {

        val sub_hash =
            SessionActor.session_mgr.refreshSession(client) match {
                case null       => ""
                case session    => session.subscription_hash
            }

        val now = System.currentTimeMillis / 1000.0

        clientActor ! JsonCallRsp( Map(
                                   "time"     -> now,
                                   "sub_hash" -> sub_hash
                                  ),
                                  null)
    }

    def auth_login(clientActor: ActorRef, client: String, params: Any) {

        logger.info("auth_login: " + client + "," + params)

        val req = JsonHelper.convert[LoginReq](params)

        val session = SessionActor.session_mgr.createSession(client, req.username, req.password)
        if (session  != null) {
            val user = UserInfo(session.username, session.name)
            logger.info(s"login successfully: $user")
            clientActor ! JsonCallRsp(user, null)
        } else {
            logger.warn(s"login failure: $params")
            clientActor ! JsonCallRsp(null, s"login failure: $params", -1000)
        }
    }

    def auth_logout(clientActor: ActorRef, client: String, params: Any) {
        SessionActor.session_mgr.destroySession(client)
        clientActor ! JsonCallRsp(true, null)
    }
    
    def onSubscribeSessionTerminatedIndReq(client: String): Unit = {
        var subscriber = this.subscriber_map.getOrElse(client, null)
        if (subscriber == null) {
            subscriber = mutable.ArrayBuffer[ActorRef]()
            this.subscriber_map += client -> subscriber
        }
        subscriber += sender()
        sender() ! SubscribeSessionTerminatedIndRsp(client, true)
    }
    
    def onCheckSession(): Unit = {
        val removed = session_mgr.check()
        for ( client <- removed) {
            val subscriber = this.subscriber_map.getOrElse(client, null)
            if(subscriber == null ) {
                logger.debug(s"Can't find ${client}'s subscriber")
            } else {
                for (sub <- subscriber) sub ! SessionTerminatedInd(client)
            }
        }
    }
}
