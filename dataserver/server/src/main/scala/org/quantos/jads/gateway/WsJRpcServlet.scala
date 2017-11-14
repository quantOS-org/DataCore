
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

import javax.servlet.annotation.WebServlet

import org.quantos.utils.jrpc.MarshallingType.MarshallingType
import org.quantos.utils.jrpc.{JRpcMessage, JsonHelper, MarshallingType, MsgPackHelper}
import org.eclipse.jetty.websocket.api.WebSocketAdapter
import org.eclipse.jetty.websocket.servlet._
import org.quantos.utils.jrpc

import scala.collection.JavaConverters._
import org.xerial.snappy.Snappy

@SuppressWarnings(Array("serial"))
@WebServlet(name = "JRpc WebSocket Servlet", urlPatterns = { Array("/ws") })
class WsJRpcServlet extends WebSocketServlet {
    override
    def configure(factory: WebSocketServletFactory ) {

        // set a 10 second timeout
        factory.getPolicy().setIdleTimeout(10000);

        factory.setCreator(new WsJRpcSocketCreator())
    }
}

class WsJRpcSocketCreator extends WebSocketCreator {

    val logger = org.slf4j.LoggerFactory.getLogger(getClass.getSimpleName)

    var curId = 1
    def getNextId = {
        curId += 1
        s"WS-$curId"
    }

    override def createWebSocket(req: ServletUpgradeRequest, rsp: ServletUpgradeResponse) : Object = {

        //for ( subprotocol <- req.getSubProtocols.asScala) {
        val protos = req.getSubProtocols.asScala.toSet

        if (protos.contains("jsonrpc.2.0.snappy") ){
            rsp.setAcceptedSubProtocol("jsonrpc.2.0.snappy")
            new WebSocketConnection (getNextId, JRpcServer.rpcserver, true, MarshallingType.Json)

        } else if ( protos.contains("jsonrpc.2.0") ){
            rsp.setAcceptedSubProtocol("jsonrpc.2.0")
            new WebSocketConnection(getNextId, JRpcServer.rpcserver, false, MarshallingType.Json)

        } else if ( protos.contains("msgpackrpc.2.0") ){
            rsp.setAcceptedSubProtocol("msgpackrpc.2.0")
            new WebSocketConnection(getNextId, JRpcServer.rpcserver, true, MarshallingType.MsgPack)

        } else {
            // No valid subprotocol in request, ignore the request
            logger.error("ERROR: un supported protocol")
            null
        }
    }
}

class WebSocketConnection(_id: String,
                          rpcserver: jrpc.JRpcServer,
                          is_binary : Boolean,
                          msg_type : MarshallingType = MarshallingType.Json)
    extends WebSocketAdapter with jrpc.Connection {


    // trait jsonrpc.Connection
    override  def id: String = _id

    override def send(msg: JRpcMessage) : Boolean = {

        val data = msg_type match {
            case MarshallingType.MsgPack => MsgPackHelper.serialize(msg)
            case MarshallingType.Json    => JsonHelper.serialize(msg).getBytes("UTF-8")
        }

        this.synchronized {
            val remote = getRemote
            if (remote == null) return false

            if (is_binary)
                remote.sendBytes( java.nio.ByteBuffer.wrap( compress(data)) )
            else
                remote.sendString(new String(data, "UTF-8"))
            true
        }
    }

    val logger = org.slf4j.LoggerFactory.getLogger("WebSocketConnection-" + id)

    override def onWebSocketClose(statusCode: Int, reason: String): Unit = {
        super.onWebSocketClose(statusCode, reason)
        logger.warn("websocket closed: " + statusCode + "," + reason)
        rpcserver.onClose(this)
    }

    override def onWebSocketConnect(sess: org.eclipse.jetty.websocket.api.Session): Unit = {
        super.onWebSocketConnect(sess)
        logger.info("websocket connected")
    }

    override def onWebSocketError(cause: Throwable): Unit = {
        super.onWebSocketError(cause)
        rpcserver.onClose(this)
    }

    override def onWebSocketText(message: String): Unit = {
        try {
            // WARNING: msg_type should be MarshallingType.json
            val msg = JsonHelper.deserialize[JRpcMessage](message)
            rpcserver.onRecv(this, msg)
        } catch {
            case t: Throwable => logger.error(t.toString)
        }
    }

    override  def onWebSocketBinary(payload: Array[Byte], offset: Int, len: Int): Unit = {
        try {
            val data = decompress(payload, offset, len)
            if (data == null || data.isEmpty) return
            val rpc_msg = data(0) match {
                case '{' =>
                    JsonHelper.deserialize[JRpcMessage]( new String(data, "UTF-8") )
                case _  =>
                    MsgPackHelper.deserialize[JRpcMessage]( data )
            }

            rpcserver.onRecv(this, rpc_msg)
        } catch {
            case t: Throwable => logger.error(t.toString)
        }
    }

    def compress(data: Array[Byte]) = {

        val maxCompressedLength = Snappy.maxCompressedLength(data.length)
        val compressed = new Array[Byte](maxCompressedLength + 1)
        compressed(0) = 'S'

        val compressedLength = Snappy.compress(data, 0, data.length, compressed, 1)

        compressed.splitAt(compressedLength + 1)._1
    }

    def decompress(bytes: Array[Byte], off: Int, len: Int): Array[Byte] = {

        if ( bytes(off) != 'S') return null

        val restored = new Array[Byte]( Snappy.uncompressedLength(bytes, 1, len - 1))
        Snappy.uncompress(bytes, 1, len - 1 , restored, 0)

        restored
    }
}




