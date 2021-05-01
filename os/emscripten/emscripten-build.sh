#!/bin/bash

BUILD_TYPE=Release
[ "$1" = "debug" ] && BUILD_TYPE=Debug
[ "$1" = "release" ] && BUILD_TYPE=Release

cd ../..

[ -z "`which emsdk`" ] && export PATH=`pwd`/../emsdk:$PATH

[ -z "`which emsdk`" ] && { echo "Put emsdk into your PATH"; exit 1 ; }

[ -z "$PATH_EMSDK" ] && PATH_EMSDK="`which emsdk | xargs dirname`"

source "$PATH_EMSDK/emsdk_env.sh"

mkdir -p build-wasm-$BUILD_TYPE
cd build-wasm-$BUILD_TYPE

[ -e build-host ] || {
	rm -rf build-host
	mkdir -p build-host
	cd build-host
	cmake ../.. -DOPTION_TOOLS_ONLY=ON || exit 1
	make -j8 tools || exit 1
	cd ..
}

embuilder build liblzma
embuilder build --lto liblzma

mkdir -p baseset
[ -e baseset/opengfx-0.6.1.tar ] || {
	wget https://cdn.openttd.org/opengfx-releases/0.6.1/opengfx-0.6.1-all.zip || exit 1
	unzip opengfx-0.6.1-all.zip || exit 1
	rm opengfx-0.6.1-all.zip
	mv opengfx-0.6.1.tar baseset/
}

[ -e Makefile ] || emcmake cmake .. -DHOST_BINARY_DIR=$(pwd)/build-host -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DOPTION_USE_ASSERTS=OFF || exit 1
emmake make -j8 VERBOSE=1 || exit 1

cp -f *.html *.js *.mem *.data *.wasm ../media/openttd.256.png ../os/emscripten/openttd.webapp /var/www/html/
