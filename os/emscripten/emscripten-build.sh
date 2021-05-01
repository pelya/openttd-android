#!/bin/bash

cd ../..

[ -z "`which emsdk`" ] && export PATH=`pwd`/../emsdk:$PATH

[ -z "`which emsdk`" ] && { echo "Put emsdk into your PATH"; exit 1 ; }

[ -z "$PATH_EMSDK" ] && PATH_EMSDK="`which emsdk | xargs dirname`"

source "$PATH_EMSDK/emsdk_env.sh"

mkdir -p build-wasm
cd build-wasm

[ -e build-host ] || {
	rm -rf build-host
	mkdir -p build-host
	cd build-host
	cmake ../.. -DOPTION_TOOLS_ONLY=ON || exit 1
	make -j8 tools || exit 1
	cd ..
}

embuilder build --lto liblzma

[ -e Makefile ] || emcmake cmake .. -DHOST_BINARY_DIR=$(pwd)/build-host -DCMAKE_BUILD_TYPE=Release -DOPTION_USE_ASSERTS=OFF || exit 1
emmake make -j8 VERBOSE=1 || exit 1

cp -f *.html *.js *.mem *.data *.wasm ../media/openttd.256.png ../os/emscripten/openttd.webapp /var/www/html/
