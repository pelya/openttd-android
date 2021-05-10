#!/bin/sh

INSTALL_PATH=`pwd`/../../../openttd-touch-webapp
MESSAGE="`git log --format=%B -n 1 HEAD`"
[ -n "$1" ] && MESSAGE="$1"

export NO_CLEAN=1

./emscripten-build.sh release $INSTALL_PATH || exit 1

cd $INSTALL_PATH || exit 1

git commit -a -m "$MESSAGE" && git push
