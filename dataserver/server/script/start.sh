#!/bin/sh
export JAVA_HOME=/opt/java/jdk/prod
export LD_LIBRARY_PATH=/opt/java/hdf-java/lib/linux/:$LD_LIBRARY_PATH
nohup bin/dataserver dev start >> log/dataserver-`date +%Y%m%d`.log&

