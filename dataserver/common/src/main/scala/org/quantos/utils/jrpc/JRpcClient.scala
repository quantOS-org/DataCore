
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

import java.util.concurrent.{ConcurrentHashMap, ConcurrentLinkedQueue}

import org.quantos.utils.jrpc.MarshallingType.MarshallingType
import org.zeromq.{ZContext, ZMQ, ZMsg}
import org.xerial.snappy.Snappy

import scala.collection.mutable.ArrayBuffer

/**
  * Created by txu on 2016/12/20.
  */
class JRpcClient(msg_type : MarshallingType = MarshallingType.MsgPack) {

    val logger = org.slf4j.LoggerFactory.getLogger("JsonRpcClient")

    private object Msg {

        class BaseMsg()
        case class RpcCallResult(time: Long, result: Any, error: JRpcError)  extends BaseMsg
        case class AsyncCall( func : ()=>Unit )                     extends BaseMsg

        val nullCallResult = RpcCallResult(0, null, null)
    }

    private val internalAsyncCallQueue = new ConcurrentLinkedQueue[Msg.AsyncCall]()

    private val syncCallWaiterMap  = new ConcurrentHashMap[Int, Msg.RpcCallResult]()
    private val asyncCallWaiterMap = new ConcurrentHashMap[Int, (String, Long)]()


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

    private var remote_sock : ZMQ.Socket = _

    private var addr: String = _
    private var clientId: String = _

    @volatile var _shouldClose = false
    def shouldClose() = _shouldClose

    private var lastHeartBeatRspTime = 0L
    private var connected = false

    var onConnected      : () => Unit = _
    var onDisconnected   : () => Unit = _
    var onNotification   : (String, Any) => Unit = _
    var onAsycCallResult : (Int, String, JRpcCallResult) => Unit = _


    private val main_thread = new Thread { override def run() { main_run() } }
    private val asynccall_thread = new Thread { override def run() { asynccall_run() } }

    main_thread.setDaemon(true)
    main_thread.start()

    asynccall_thread.setDaemon(true)
    asynccall_thread.start()

    private var _callId = 0

    private def nextCallId(): Int = {
        (nextCallId _) .synchronized{
            _callId += 1
            _callId
        }
    }

    private def asynccall_run() {

        while (!shouldClose) {

            val msg =
                internalAsyncCallQueue.synchronized{
                    if (internalAsyncCallQueue.isEmpty) {
                        internalAsyncCallQueue.wait(100)
                    }
                    internalAsyncCallQueue.poll()
                }

            try {
                if (msg != null )
                    msg.func()
            } catch {
                case t: Throwable => logger.warn("catch exception in asyncall: " + t.getMessage)
            }
        }
    }

    private def internalAsynccall( func: () => Unit ) {
        internalAsyncCallQueue.add( Msg.AsyncCall(func))
    }

    private def main_run() {

        var lastHeartBeatTime = System.currentTimeMillis()
        var items =  Array( new ZMQ.PollItem(pull_sock, ZMQ.Poller.POLLIN) )

        var lastAsyncCallCheckTime = System.currentTimeMillis

        while (!shouldClose) {

            try {
                ZMQ.poll(items, 100)

                if (items(0).isReadable) {
                    val data = pull_sock.recv()
                    data(0) match {
                        case 'S'    =>  doSend(pull_sock.recv())
                        case 'C'    =>  doConnect()
                        case 'D'    =>
                    }
                }

                if (items.length == 1 ) {
                    if (remote_sock != null)
                        items =  Array( new ZMQ.PollItem(pull_sock,   ZMQ.Poller.POLLIN),
                            new ZMQ.PollItem(remote_sock, ZMQ.Poller.POLLIN) )

                } else {
                    if (items(1).isReadable)
                        doRecv()

                    if (System.currentTimeMillis() - lastHeartBeatTime > 1000) {
                        lastHeartBeatTime = System.currentTimeMillis()
                        doSendHeartBeat()
                    }

                    if ( connected ) {
                        if (System.currentTimeMillis() - lastHeartBeatRspTime > 3000 ) {
                            this.connected = false
                            if (this.onDisconnected != null)
                                internalAsynccall(this.onDisconnected)
                        }
                    }
                }

                if (System.currentTimeMillis - lastAsyncCallCheckTime > 1000) {
                    lastAsyncCallCheckTime = System.currentTimeMillis
                    checkAsyncCallTimeout()
                }
            }catch {
                case t:Throwable => logger.error("catch exception in main_thread: " + t.getMessage)
            }
        }
    }

    private def doRecv() {

        try {
            val data = this.remote_sock.recv(ZMQ.DONTWAIT)
            if (data == null) return
            val msg = unpack(data)

            if (msg.method != null && msg.method == ".sys.heartbeat") {
                lastHeartBeatRspTime = System.currentTimeMillis()
                if ( ! this.connected ) {
                    this.connected = true
                    if (this.onConnected != null )
                        internalAsynccall(this.onConnected)
                }

            } else if(msg.id != null && !msg.id.isEmpty ) {
                // Call Result
                val callId = msg.id.toInt

                val now = System.currentTimeMillis
                syncCallWaiterMap.synchronized {
                    if (syncCallWaiterMap.containsKey(callId) ) {
                        syncCallWaiterMap.put(callId, Msg.RpcCallResult(now, error = msg.error, result = msg.result) )
                        syncCallWaiterMap.notify()
                    }
                }

                asyncCallWaiterMap.synchronized {
                    if (asyncCallWaiterMap.containsKey(callId) ) {
                        asyncCallWaiterMap.remove(callId)
                        internalAsynccall( () => this.onAsycCallResult(callId, msg.method, JRpcCallResult(result = msg.result, error = msg.error) ))
                    }
                }
            } else {
                // Notification message
                if (msg.method != null && msg.result != null && onNotification != null )
                    if (onNotification != null)
                        internalAsynccall( () => onNotification(msg.method, msg.result) )
            }
        } catch {
            case t: Throwable => t.printStackTrace()
        }

    }

    def isConnected = this.connected

    def connect(clientId: String, addr: String): Boolean = {
        if (addr == null || addr.isEmpty) return false
        this.addr = addr

        if (clientId == null )
            this.clientId = ""
        else
            this.clientId = clientId

        push_sock.synchronized{ push_sock.send("C".getBytes("UTF-8")) }

        true
    }

    private def doConnect() {

        val sock = ctx.createSocket(ZMQ.DEALER)
        sock.setReceiveTimeOut(2000)
        sock.setSendTimeOut(2000)
        sock.setLinger(0)

        val random = new java.util.Random(System.currentTimeMillis() + clientId.hashCode())
        val id =
            if (clientId != "")
                clientId + "$" + Math.abs(random.nextLong())
            else
                Math.abs(random.nextLong()) + "$" + Math.abs(random.nextLong())

        sock.setIdentity(id.getBytes)
        sock.connect(addr)

        this.remote_sock = sock
    }

    def close() {
        _shouldClose = true
        push_sock.synchronized{ push_sock.send("D".getBytes("UTF-8")) }
    }

    def pack(msg : JRpcMessage) = {
        msg_type match {
            case MarshallingType.MsgPack =>
                val data = MsgPackHelper.serialize(msg)
                if (data.length > 1000 )
                    Array[Byte]('S') ++ Snappy.compress(data)
                else
                    Array[Byte](0) ++ data

            case MarshallingType.Json    => JsonHelper.serialize(msg).getBytes("UTF-8")
        }
    }

    def unpack(data: Array[Byte]) : JRpcMessage = {
        msg_type match {
            case MarshallingType.Json    =>
                JsonHelper.deserialize[JRpcMessage](new String(data, "UTF-8") )

            case MarshallingType.MsgPack =>
                data(0) match {
                case '\0' =>
                    MsgPackHelper.deserialize[JRpcMessage](data.tail)
                case 'S' =>
                    val udata = Snappy.uncompress(data.tail)
                    MsgPackHelper.deserialize[JRpcMessage](udata)

            }
        }
    }

    def call(method: String, params: Any, timeout: Int = 60000) : JRpcCallResult = {

        val callId = nextCallId()

        if (timeout != 0 )
            syncCallWaiterMap.put(callId, Msg.nullCallResult )

        val msg = JRpcMessage( jsonrpc = "2.0",
            method = method,
            params = params,
            id = callId.toString,
            time = System.currentTimeMillis)

        val data = pack(msg)

        val zmsg = new ZMsg
        zmsg.add("S")
        zmsg.add(data)

        push_sock.synchronized{
            zmsg.send(push_sock)
        }

        if (timeout ==0 ) return null

        val end = System.currentTimeMillis() + timeout

        var returnResult: JRpcCallResult = null
        while( returnResult == null ) {
            syncCallWaiterMap.synchronized{
                syncCallWaiterMap.getOrDefault(callId, Msg.nullCallResult) match {
                    case Msg.nullCallResult =>
                        val now = System.currentTimeMillis()
                        if ( now < end ) {
                            syncCallWaiterMap.wait( end - now )
                        } else {
                            syncCallWaiterMap.remove(callId)
                            returnResult = JRpcCallResult(null, JRpcError(error = -1, message="TIMEOUT"))
                        }
                    case cr =>
                        syncCallWaiterMap.remove(callId)
                        //println("notify time: " + (System.currentTimeMillis() - cr.time))
                        returnResult = JRpcCallResult(result = cr.result, error = cr.error)
                }
            }
        }
        returnResult
    }

    def asyncCall(method: String, params: Any, timeout: Int = 6000) : Int = {

        val callId = nextCallId()

        asyncCallWaiterMap.put(callId, (method, System.currentTimeMillis + timeout))

        val msg = JRpcMessage( jsonrpc = "2.0",
            method = method,
            params = params,
            id = callId.toString,
            time = System.currentTimeMillis)

        val data = pack(msg)

        val zmsg = new ZMsg
        zmsg.add("S")
        zmsg.add(data)

        push_sock.synchronized{
            zmsg.send(push_sock)
        }
        callId
    }

    private def doSend(utf8_data: Array[Byte]) = {
        try {
            //logger.info("SEND " + new String(utf8_data, "UTF-8"))
            remote_sock.send(utf8_data)
        } catch {
            case t: Throwable => t.printStackTrace()
        }
    }

    private def doSendHeartBeat() {

        val msg = JRpcMessage( jsonrpc = "2.0",
            method  = ".sys.heartbeat",
            params  = Map( "time" -> System.currentTimeMillis ),
            id      = this.nextCallId().toString,
            time    = System.currentTimeMillis)

        val data = pack(msg)

        doSend(data)
    }

    private def checkAsyncCallTimeout(): Unit = {
        val now = System.currentTimeMillis

        val dead_id = ArrayBuffer[Int]()
        asyncCallWaiterMap.synchronized {
            val e = asyncCallWaiterMap.keys()
            while (e.hasMoreElements) {
                val id = e.nextElement()
                val (method, timeout) = asyncCallWaiterMap.get(id)
                if (timeout <= now)
                    dead_id += id
            }
        }

        for ( id <- dead_id) {
            val (method, _) = asyncCallWaiterMap.remove(id)
            internalAsynccall( () => this.onAsycCallResult(id, method, JRpcCallResult(result = null, error = JRpcError(error = -1, message="TIMEOUT"))))
        }
    }
}
