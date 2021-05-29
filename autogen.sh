#!/usr/bin/env bash

set -e

DIR=`dirname $0`

args=$@

if [ -e /usr/bin/python2 ]; then
    args="$args PYTHON=/usr/bin/python2"
fi
AUTOPOINT='intltoolize' autoreconf --install --force $DIR
$DIR/configure $args
