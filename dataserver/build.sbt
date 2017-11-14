name          := "dataserver"

scalaVersion    := "2.11.8"

javacOptions ++= Seq("-encoding", "UTF-8")

lazy val common = project in file("common")
lazy val server = project in file("server")    dependsOn(common)

val runServer = taskKey[Unit]("Runs server")

runServer := (run in Compile in server).toTask("").value

lazy val root = project.in( file("."))
	.aggregate(common, server)
	.settings( aggregate in update := false	)

