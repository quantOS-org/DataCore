#!/bin/bash
name='dataserver'
echo stop $name
pid=`ps -ef | grep  $name | grep -v grep | grep -v stop| awk '{ print $2 }'`
kill -9 $pid  2>/dev/null

