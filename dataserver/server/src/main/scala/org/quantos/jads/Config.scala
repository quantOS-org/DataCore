
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

package org.quantos.jads

import org.quantos.utils.jrpc.JsonHelper

import scala.io.Source

object Config {

    val logger = org.slf4j.LoggerFactory.getLogger(getClass.getSimpleName)

    case class MdlinkConfig(
            addr        : String,
            pub_addr    : String
    )

    case class QmsConfig (
            addr        : String
    )

    case class FrontEndConfig(
            zmqrpc_addr : String
    )

    case class HttpServerConfig(
            port : Int,
            doc_root  : String
    )

    case class AllConfig (
        mdlink      : MdlinkConfig,
        qms         : QmsConfig,
        frontend    : FrontEndConfig,
        http_server : HttpServerConfig
    )

    private var _env = ""

    def init(env: String) {
        _env = env
        logger.info(s"=== ${_env} ENVIRONMENT ===")
        _config = load()
    }

    private var _config: AllConfig = _

    def conf = _config
    def env  = _env

    def load(): AllConfig = {
        val file = s"etc/dataserver-${_env}.conf"
        logger.info("Load configure file: " + file)
        val text = Source.fromFile(file).mkString
        JsonHelper.deserialize[AllConfig](text)
    }


    object akka {

        import _root_.akka.actor.ActorSystem
        import _root_.com.typesafe.config.ConfigFactory


        val conf = ConfigFactory.load(ConfigFactory.parseString(
"""
akka{
   loggers = ["akka.event.slf4j.Slf4jLogger"]
   loglevel = "INFO"
   logging-filter = "akka.event.slf4j.Slf4jLoggingFilter"
}
"""))
        val system = ActorSystem("dataserver") //"abc", conf);
    }

}
