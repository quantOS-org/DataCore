
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

package org.quantos.utils.jrpc

import java.nio.ByteBuffer

import org.zeromq.{ZContext, ZMQ, ZMsg}
import java.util.concurrent.ConcurrentHashMap

import scala.collection.mutable
import scala.concurrent.Future
import scala.concurrent.ExecutionContext.Implicits.global
import scala.collection.JavaConverters._
import org.xerial.snappy.Snappy

/**
  * Created by txu on 2016/12/20.
  */

object MarshallingType extends Enumeration{
    type MarshallingType = Value
    val Json    = Value(0, "Json")
    val MsgPack = Value(1, "MsgPack")
}

trait Connection {

    def id: String
    def send(msg : JRpcMessage) : Boolean
}

trait JRpcServerCallback {

    def onCall(conn_id: String , method: String, params: Any) : Future[JRpcCallResult]

    def onClose(conn_id: String)
}

class JRpcServer {

    val logger = org.slf4j.LoggerFactory.getLogger(getClass.getSimpleName)

    var callback : JRpcServerCallback = _

    val connMap = new ConcurrentHashMap[String, Connection]().asScala

    def _getConn(conn_id: String) = {
        connMap.getOrElse(conn_id, null)
    }

    def notify(conn_id: String, evt: String, data: Any): Boolean = {

        _getConn(conn_id) match {
            case null => false
            case conn =>
                val msg = JRpcMessage( jsonrpc = "2.0",
                    method = evt,
                    result = data,
                    time = System.currentTimeMillis)

                conn.send(msg)
        }
    }

    def onClose(conn : Connection) = {

        connMap.remove(conn.id)

        this.callback.onClose(conn.id)
    }

    /**
      * Connection receives data and call this function to process incoming request.
      *
      * @param conn
      * @param msg
      */
    def onRecv(conn: Connection, msg : JRpcMessage) : Unit =  {

        try {
            connMap.putIfAbsent( conn.id, conn)

            // Server doesn't receive notification
            if (msg.id == null) return

            // process Call
            val callId = msg.id.toInt

            if (msg.method != null && msg.method.nonEmpty && callback != null ) {

                val f = callback.onCall(conn.id, msg.method, msg.params)

                f.onSuccess { case r =>
                    val rsp = JRpcMessage(jsonrpc = "2.0",
                        method  = msg.method,
                        result  = r.result,
                        error   = r.error,
                        id      = msg.id,
                        time    = System.currentTimeMillis)

                    conn.send( rsp )
                }

                f.onFailure { case _ =>
                    val rsp = JRpcMessage(jsonrpc = "2.0",
                        method  = msg.method,
                        error   = JRpcError(-1, "FIXME", null),
                        id      = msg.id,
                        time    = System.currentTimeMillis)

                    conn.send( rsp )
                }

            } else {
                // FIXME:
                val rsp = JRpcMessage(jsonrpc = "2.0",
                    method  = msg.method,
                    error   = JRpcError(-1, "FIXME", null),
                    id      = msg.id,
                    time    = System.currentTimeMillis)

                conn.send( rsp )
            }
        } catch {
            case t: Throwable => logger.error("onRecv error", t)
        }
    }
}

class ZmqRpcServerConnection(server: ZmqJRpcServer, rpcserver: JRpcServer, client_id: String, addr: Array[Byte]) extends Connection {

    var msg_type = MarshallingType.Json

    def id = this.client_id

    def send(msg: JRpcMessage) : Boolean = {

        val data = msg_type match {
            case MarshallingType.MsgPack =>
                val d = MsgPackHelper.serialize(msg)
                if (d.length < 500) {
                    Array[Byte](0) ++ d
                } else {
                    val tmp = new Array[Byte]( 1 + Snappy.maxCompressedLength(d.length))
                    tmp(0) = 'S'
                    val len = Snappy.compress(d, 0, d.length, tmp, 1)
                    tmp.splitAt(len+1)._1
                }

            case MarshallingType.Json    => JsonHelper.serialize(msg).getBytes("UTF-8")
        }
        server.send(data, addr)
        true
    }

    def onRecv(data: Array[Byte]) = {
        val rpc_msg = data(0) match {
            case '\0'  =>
                // format: '\0' + data serialized by msgpack
                this.msg_type = MarshallingType.MsgPack
                MsgPackHelper.deserialize[JRpcMessage]( data.splitAt(1)._2 )

            case 'S' =>
                // format: 'S' + bytes which serialized by msgpack then compressed by snappy
                this.msg_type = MarshallingType.MsgPack
                val uncompressed_data = new Array[Byte](Snappy.uncompressedLength(data, 1, data.length-1))
                Snappy.uncompress(data, 1, data.length-1, uncompressed_data, 0)
                MsgPackHelper.deserialize[JRpcMessage]( uncompressed_data )

            case '{' =>
                // Json string
                this.msg_type = MarshallingType.Json
                JsonHelper.deserialize[JRpcMessage]( new String(data, "UTF-8"))
            case _ =>
                server.logger.error("Unknown raw data format")
                null
        }

        if (rpc_msg != null)
            rpcserver.onRecv(this, rpc_msg)
        else
            server.logger.error("Unknown data format")
    }

    var last_time = System.currentTimeMillis
}

class ZmqJRpcServer(rpcserver: JRpcServer ) {

    val logger = org.slf4j.LoggerFactory.getLogger("ZmqJRpcServer")

    private val ctx = new  ZContext

    private val push_sock = {
        val sock = ctx.createSocket(ZMQ.PUSH)
        sock.bind("inproc://send_msg")
        sock
    }

    private val pull_sock = {
        val sock = ctx.createSocket(ZMQ.PULL)
        sock.connect("inproc://send_msg")
        sock
    }

    private var remote_sock: ZMQ.Socket = _

    private var addr: String = _

    @volatile
    var _shouldClose = false

    def shouldClose() = _shouldClose

    private val main_thread =  {
        val td = new Thread { override def run() { main_run() } }
        td.setDaemon(true)
        td.start()
        td
    }


    private def main_run() {

        val clientConnMap = mutable.HashMap[String, ZmqRpcServerConnection]()

        var items =  Array( new ZMQ.PollItem(pull_sock, ZMQ.Poller.POLLIN) )

        while (true) {
            try {
                ZMQ.poll(items, 100)

                if (items(0).isReadable ) {
                    val msg = ZMsg.recvMsg(pull_sock)
                    if (msg.size() == 2) {
                        if (remote_sock != null) {
                            //println("SEND TO " + new String(msg.getFirst.getData, "UTF-8") + " " + msg.getLast.size())
                            msg.send(remote_sock)
                        }
                    } else {
                        new String(msg.getFirst.getData, "UTF-8") match {
                            case "LISTEN"  => doListen()
                            case "CLOSE"   =>
                            case x => logger.error( s"unkown commond $x")
                        }
                    }
                }

                if (items.length ==  1) {
                    if (this.remote_sock != null ) {
                        items =  Array(
                            new ZMQ.PollItem(pull_sock,   ZMQ.Poller.POLLIN),
                            new ZMQ.PollItem(remote_sock, ZMQ.Poller.POLLIN)
                        )
                    }

                } else if (items.length == 2 && items(1).isReadable) {
                    val msg = ZMsg.recvMsg(remote_sock)
                    if (msg.size() == 2 ){
                        val identity = msg.getFirst
                        val data = msg.getLast.getData

                        val client_id = {
                            val str = new String(identity.getData,"UTF-8")
                            if (str.contains('$')) {
                                str.split('$').head
                            } else {
                                str
                            }
                        }

                        val addr = identity.getData

                        var conn = clientConnMap.getOrElse(client_id, null)
                        if (conn == null) {
                            conn = new ZmqRpcServerConnection(this, rpcserver, client_id, addr)
                            clientConnMap += client_id -> conn
                        }

                        conn.onRecv(data)

                    }
                }
            } catch {
                case t: Throwable => logger.error("main_run exception", t)
            }
        }
    }

    def listen(addr: String): Unit = {
        this.addr = addr
        this.push_sock.send("LISTEN")
    }

    private def doListen() {

        if (this.remote_sock != null) return

        val sock = ctx.createSocket(ZMQ.ROUTER)
        sock.setReceiveTimeOut(1000)
        sock.setSendTimeOut(2000)
        sock.setLinger(0)

        sock.bind(this.addr)

        this.remote_sock = sock
    }

    def close() {
        _shouldClose = true
        this.push_sock.send("CLOSE")
    }

    def send(data: Array[Byte], addr: Array[Byte] ) {

        try {
            val msg = new ZMsg
            msg.add(addr)
            msg.add(data)

            push_sock.synchronized( msg.send( push_sock ) )

        }catch {
            case t : Throwable => logger.error("send error: " + t)//.getMessage)
        }
    }
}
