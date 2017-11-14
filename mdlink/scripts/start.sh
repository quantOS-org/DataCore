#!/bin/bash
cd `dirname $0`
cd ..
pwd
. scripts/set-env.sh
jztsctrl qms2 qms2 start
sleep 3m
jztsctrl mdlink2 mdlink2 start
sleep 1m
jztsctrl mdlink_ctp future1 start
jztsctrl mdlink_tdf stock1 start
