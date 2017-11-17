
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

import scala.concurrent.duration.Duration
import scala.concurrent.{Await, Future}
import scala.concurrent.Await
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.duration.DurationInt
import akka.util.Timeout
import akka.actor.Props
import akka.pattern.ask
import org.quantos.jads.Config
import org.quantos.jads.services.jsi.JsiService
import org.quantos.jads.services.jsq.JsqService
import org.quantos.utils.jrpc.{JRpcCallResult, JRpcError}
import org.quantos.utils.jrpc


object JRpcServer {

    lazy val rpcserver = new jrpc.JRpcServer

    case class JsonCallReq(client: String, method: String, params: Any)

    case class JsonCallRsp(result: Any, message: String = "", error_code: Int = 0)

    lazy val session_actor = Config.akka.system.actorOf(Props[SessionActor])

    def init() {
        rpcserver.callback = new jrpc.JRpcServerCallback {
            def onCall(conn_id: String, method: String, params: Any) : Future[jrpc.JRpcCallResult] = {

                val target_actor =
                    if (method.startsWith(".sys") || method.startsWith("auth")) session_actor
                    else if (method.startsWith("jsq."))  JsqService.actor
                    else if (method.startsWith("jsi."))  JsiService.actor
                    else null

                Future {
                    val session = SessionActor.session_mgr.getSession(conn_id)
                    if (target_actor == null) {
                        JRpcCallResult(null, JRpcError(message = "unknown service", error = -1))

                    } else if (target_actor != session_actor ) {
                        if(session == null) {
                            JRpcCallResult(null, JRpcError(message = "no privilege", error = -1))
                        } else {
                            val msg =  SessionActor.session_mgr.checkFlowControl(session)
                            if(msg != "") {
                                JRpcCallResult(null, JRpcError(message = "query too frequently", error = -1))
                            } else {
                                implicit val timeout = Timeout(65 seconds)
                                val f = (target_actor ? JsonCallReq(conn_id, method, params)).mapTo[JsonCallRsp]
                                val r = Await.result(f, Duration.Inf)
                                JRpcCallResult(r.result, JRpcError(message = r.message, error = r.error_code))
                            }
                        }
                        

                    } else {

                        // XXX: Timeout should be bigger than RSP Timer in this actor
                        implicit val timeout = Timeout(65 seconds)
                        val f = (target_actor ? JsonCallReq(conn_id, method, params)).mapTo[JsonCallRsp]
                        val r = Await.result(f, Duration.Inf)

                        JRpcCallResult(r.result, JRpcError(message = r.message, error = r.error_code))
                    }
                }
            }

            def onClose(conn_id: String) = {
                // TODO:
            }
        }
    }
}

