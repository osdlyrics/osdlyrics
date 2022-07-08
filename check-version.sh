#!/usr/bin/env sh

GIT="${GIT:-git}"
OL_VERSION="${OL_VERSION:-0.5.11}"

if [ "$("$GIT" rev-parse --show-prefix 2>/dev/null)" ] ||
   ! "$GIT" describe --always --tags 2>/dev/null; then
	echo "$OL_VERSION"
fi
