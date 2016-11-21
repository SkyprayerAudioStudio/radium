#!/bin/sh

set -e


THIS_DIR=$(dirname $(readlink -f $0))
XCB_LIB_DIR=$THIS_DIR/packages/libxcb-1.12/src/.libs

if ! file $XCB_LIB_DIR ; then
    echo "Unable to find directory $XCB_LIB_DIR"
    exit -1
fi

export LD_LIBRARY_PATH=$LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}

LD_LIBRARY_PATH=$LD_LIBRARY_PATH $THIS_DIR/radium_linux.bin $@

