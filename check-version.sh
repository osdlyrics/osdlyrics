#!/usr/bin/env sh

set -e

DIR=`dirname $0`
GIT="${GIT:-git}"
OL_VERSION="${GIT:-0.5.11}"

if test -d '.git' && command -v "$GIT" &> /dev/null; then
	"$GIT" describe --always --tags
else
	echo "$OL_VERSION"
fi

