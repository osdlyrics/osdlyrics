#!/usr/bin/env sh

set -e

DIR=`dirname $0`
GIT="${GIT:-git}"

if test -d '.git' && command -v "$GIT" &> /dev/null; then
	"$GIT" describe --always --tags
else
	echo 0.5.11
fi

