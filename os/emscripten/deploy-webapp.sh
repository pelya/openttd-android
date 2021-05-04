#!/bin/sh

INSTALL_PATH=`pwd`/../../../openttd-touch-webapp

./emscripten-build.sh release $INSTALL_PATH || exit 1

cd $INSTALL_PATH || exit 1

git commit -a * && git push
