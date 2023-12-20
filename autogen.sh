#!/usr/bin/env bash

set -e

DIR=`dirname $0`

args=$@

AUTOPOINT='intltoolize' autoreconf --install --force $DIR
$DIR/configure $args
