#!/bin/sh

#export JZS_HOME=$(dirname $(dirname $(readlink -f "$0")) )
export JZS_HOME=`pwd`

export LD_LIBRARY_PATH=$JZS_HOME/lib:$LD_LIBRARY_PATH

export PATH=$JZS_HOME/bin:$JZS_HOME/scripts:$PATH

