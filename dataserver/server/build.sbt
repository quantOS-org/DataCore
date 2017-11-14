organization    := "org.quantos"

name            := "dataserver"

version         := "0.5.0"

scalaVersion    := "2.11.8"

resolvers       += "Local Maven Repository" at "file:///D:/java/maven/repo"
resolvers       += "Scalaz Bintray Repo"    at "http://dl.bintray.com/scalaz/releases"

val akkaVersion  = "2.4.11"

libraryDependencies += "ch.qos.logback" % "logback-core" % "1.1.7"
libraryDependencies += "ch.qos.logback" % "logback-classic" % "1.1.7"
libraryDependencies += "org.slf4j" % "slf4j-api" % "1.7.20"

libraryDependencies += "com.typesafe.akka"  %% "akka-actor"              % akkaVersion
libraryDependencies += "com.typesafe.akka"  %% "akka-slf4j"              % akkaVersion
libraryDependencies += "com.typesafe.akka"  %% "akka-testkit"            % akkaVersion

libraryDependencies += "org.scala-lang" % "scala-compiler"    % "2.11.8"

libraryDependencies += "com.google.protobuf" % "protobuf-java" % "2.5.0"
libraryDependencies += "org.zeromq"          % "jeromq"        % "0.3.5"

libraryDependencies += "org.eclipse.jetty" % "jetty-server"    % "9.3.13.v20161014"
libraryDependencies += "org.eclipse.jetty.websocket" % "javax-websocket-server-impl" % "9.3.13.v20161014"

libraryDependencies += "ws.wamp.jawampa" % "jawampa-core" % "0.4.1"
libraryDependencies += "ws.wamp.jawampa" % "jawampa-netty" % "0.4.1"

libraryDependencies += "com.fasterxml.jackson.core"    % "jackson-core"         % "2.9.0"
libraryDependencies += "com.fasterxml.jackson.core"    % "jackson-annotations"  % "2.9.0"
libraryDependencies += "com.fasterxml.jackson.module" %% "jackson-module-scala" % "2.9.0"
libraryDependencies += "com.fasterxml.jackson.dataformat" % "jackson-dataformat-csv" % "2.9.0"

libraryDependencies += "org.scalactic" %% "scalactic" % "3.0.0"
libraryDependencies += "org.scalatest" %% "scalatest" % "3.0.0" % "test"
libraryDependencies += "org.scalamock" %% "scalamock-scalatest-support" % "3.3.0" % "test"

libraryDependencies += "commons-cli" % "commons-cli" % "1.3"
libraryDependencies += "org.apache.commons" % "commons-dbcp2" % "2.0"

libraryDependencies += "org.xerial.snappy" % "snappy-java" % "1.1.2.6"
libraryDependencies += "com.zaxxer" % "HikariCP" % "2.6.1"

packAutoSettings

packMain        := Map("dataserver" -> "org.quantos.jads.Boot")

packJvmOpts     := Map("dataserver" -> Seq(
  "-Djava.library.path=${PROG_HOME}/lib",
  "-server",
  "-Xms8g",
  "-Xmx8g",
  "-XX:NewRatio=2",
  "-XX:SurvivorRatio=8",
  "-XX:+PrintGCDetails"
))

packGenerateWindowsBatFile := true
packResourceDir += (baseDirectory.value / "etc" -> "etc")
packResourceDir += (baseDirectory.value / "log" -> "log")
packResourceDir += (baseDirectory.value / "lib" -> "lib")
packResourceDir += (baseDirectory.value / "script" -> "script")
