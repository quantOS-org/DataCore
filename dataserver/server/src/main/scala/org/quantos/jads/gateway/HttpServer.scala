
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

import java.io.File

import org.eclipse.jetty.server.Server
import org.eclipse.jetty.server.handler.{DefaultHandler, HandlerList, ResourceHandler}
import org.eclipse.jetty.servlet.{ServletContextHandler, ServletHolder}
import org.quantos.jads.Config

object HttpServer {

    val logger = org.slf4j.LoggerFactory.getLogger(getClass.getSimpleName)

    def start() = {

        val absPath = {
            val dir = new File(Config.conf.http_server.doc_root)
            dir.getAbsolutePath
        }

        logger.info("Start Jetty web server (" + Config.conf.http_server.port + ", " + absPath + ")")

        val server = new Server(Config.conf.http_server.port)

        val handler = new ResourceHandler()
        handler.setDirectoriesListed(true)
        handler.setWelcomeFiles(Array("index.html"))
        handler.setResourceBase(absPath)
        handler.setMinMemoryMappedContentLength(-1)

        val context = new ServletContextHandler(ServletContextHandler.SESSIONS)
        context.setContextPath("/")
        context.addServlet(new ServletHolder(classOf[WsJRpcServlet]), "/ws/*")

        val handlers = new HandlerList()
        handlers.setHandlers(Array(handler, context, new DefaultHandler()))

        server.setHandler(handlers)

        server.start()
    }

}


