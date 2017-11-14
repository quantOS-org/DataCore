organization    := "org.quantos"

name            := "common"

version         := "0.5.0"

scalaVersion    := "2.11.8"

resolvers       += "Local Maven Repository" at "file:///D:/java/maven/repo"
resolvers       += "Scalaz Bintray Repo"    at "http://dl.bintray.com/scalaz/releases"


libraryDependencies += "ch.qos.logback" % "logback-core" % "1.1.7"
libraryDependencies += "ch.qos.logback" % "logback-classic" % "1.1.7"
libraryDependencies += "org.slf4j" % "slf4j-api" % "1.7.20"

//libraryDependencies += "org.scala-lang" % "scala-compiler"    % "2.11.8"

libraryDependencies += "org.zeromq"          % "jeromq"        % "0.3.5"


libraryDependencies += "com.fasterxml.jackson.core"    % "jackson-core"         % "2.7.3"
libraryDependencies += "com.fasterxml.jackson.core"    % "jackson-annotations"  % "2.7.3"
libraryDependencies += "com.fasterxml.jackson.module" %% "jackson-module-scala" % "2.7.3"

libraryDependencies += "com.google.protobuf" % "protobuf-java" % "2.5.0"

libraryDependencies += "org.msgpack" % "jackson-dataformat-msgpack" % "0.8.12"

libraryDependencies += "org.xerial.snappy" % "snappy-java" % "1.1.2.6"

packAutoSettings
packMain        := Map("pur_stk_test" -> "test.Tdlink2Client")

packGenerateWindowsBatFile := true
