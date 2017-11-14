
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

package test


import org.quantos.utils.jrpc._

import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global

/**
  * Created by txu on 2017/6/7.
  */
object JsonRpcTest extends App {

    class MyServerCallback(server: JRpcServer) extends JRpcServerCallback {

        var count = 0
        var recv_time = 0L
        def onCall(conn_id: String , method: String, params: Any) : Future[JRpcCallResult] = {
            //println("onCall:", method, params)
            recv_time += System.nanoTime - params.asInstanceOf[Map[String,Long]]("send_time")
            count += 1
            if ( count == 1000) {
                println("recv_time:", recv_time/count)
                count = 0
                recv_time = 0
            }

            Future {
                new JRpcCallResult (Map[String, Long] ( "send_time" -> System.nanoTime))
            }
        }

        def onClose(conn_id: String) = {

        }
    }

    val server = new JRpcServer()
    server.callback = new MyServerCallback(server)

    val zmq_server = new ZmqJRpcServer(server)
    zmq_server.listen("tcp://localhost:1234")


    Thread.sleep(1000)
    val client = new JRpcClient(MarshallingType.MsgPack)
    client.connect("", "tcp://localhost:1234")
    for ( _ <- 0 until 100) {
        val loop = 1000
        val t = System.nanoTime()
        for ( _ <- 0 until loop ) {
            val ret = client.call("hello", Map("send_time" -> System.nanoTime))
        }
        println("time:", (System.nanoTime() - t) / loop)
    }
}
