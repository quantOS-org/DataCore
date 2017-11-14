
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

import org.quantos.jads.utils.{BaseData, SharedMdlinkClient}
import org.apache.commons.cli.{DefaultParser, HelpFormatter, Options}

object Boot extends App{

    def parse() : (String, String, Array[String]) = {

        val options = new Options()
        options.addOption( "v", "version", false, "show version" )
        options.addOption( "h", "help", false, "show help" )

        var pos = args.length
        for ( i <- args.indices if pos == args.length) {
            if (args(i) ==  "--")
                pos = i
        }

        val (main_args, tmp_args) = args.splitAt(pos)
        val cmd_args = if (tmp_args.isEmpty) tmp_args else tmp_args.splitAt(1)._2

        val parser = new DefaultParser()
        val main_line = parser.parse(options, main_args)
        val opts = main_line.getOptions

        if (main_line.hasOption("v")) {
            println(s"current version: ${getClass.getPackage.getImplementationVersion}")
            System.exit(0)
        }

        if (main_line.getArgList.size() != 2) {
            println("wrong command " + main_line.getArgList)
            new HelpFormatter().printHelp( "dataserver <env>[dev|uat|prod1|idc] <cmd>[start|init|import|speedup]", options )
            System.exit(0)
        }

        (main_line.getArgList.get(0), main_line.getArgList.get(1), cmd_args)
    }

    def start(args: Array[String]): Boolean = {

        BaseData.init()

        if(Config.conf.mdlink != null) {
            SharedMdlinkClient.start(Config.conf.mdlink.addr)
        }

        services.jsi.JsiService.start()
        services.jsq.JsqService.start()

        // Start gateway in the last
        gateway.JRpcServer.init()
        gateway.ZmqJRpcServer.init()
        gateway.HttpServer.start()

        while (true) {
            try {
                Thread.sleep(100)
            }catch{
                case _: Throwable => println("Exception")
            }
        }

        true
    }

    val (env, cmd, cmd_args) = parse()

    Config.init(env)

    val result = cmd match {
        case "start"    => start(cmd_args)
        case _ => println("unkown command. only support [start, init, import]"); false
    }

    if(!result)
        System.exit(1)

}
