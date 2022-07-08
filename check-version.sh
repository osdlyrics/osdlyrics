#!/usr/bin/env sh

DIR=`dirname $0`
GIT="${GIT:-git}"
OL_VERSION="${OL_VERSION:-0.5.11}"

if ! "$GIT" describe --always --tags 2>/dev/null; then
	echo "$OL_VERSION"
fi
