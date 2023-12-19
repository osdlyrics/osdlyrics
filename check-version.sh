#h!/usr/bin/env sh

GIT="${GIT:-git}"
OL_VERSION="${OL_VERSION:-0.5.15}"

if [ ! "$("$GIT" rev-parse --show-prefix 2>/dev/null)" ]; then
  _GIT_VERSION="$("$GIT" describe --always --tags 2>/dev/null)"
  if [ "$_GIT_VERSION" ]; then
    echo "$_GIT_VERSION"
  else
    echo "$OL_VERSION"
  fi
else
  echo "$OL_VERSION"
fi
