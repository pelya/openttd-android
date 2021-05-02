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
	mv opengfx-0.6.1.tar baseset/ || exit 1
}

[ -e baseset/opensfx-1.0.1.tar ] || {
	wget https://cdn.openttd.org/opensfx-releases/1.0.1/opensfx-1.0.1-all.zip || exit 1
	unzip opensfx-1.0.1-all.zip || exit 1
	rm opensfx-1.0.1-all.zip
	mv opensfx-1.0.1.tar baseset/ || exit 1
}

[ -e baseset/openmsx-0.4.0.tar ] || {
	wget https://cdn.openttd.org/openmsx-releases/0.4.0/openmsx-0.4.0-all.zip || exit 1
	unzip openmsx-0.4.0-all.zip || exit 1
	rm openmsx-0.4.0-all.zip
	mv openmsx-0.4.0.tar baseset/ || exit 1
}

[ -e TimGM6mb.sf2 ] || {
	wget 'https://sourceforge.net/p/mscore/code/HEAD/tree/trunk/mscore/share/sound/TimGM6mb.sf2?format=raw' || exit 1
}

[ -e Makefile ] || emcmake cmake .. -DHOST_BINARY_DIR=$(pwd)/build-host -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DOPTION_USE_ASSERTS=OFF || exit 1
emmake make -j8 VERBOSE=1 || exit 1

cp -f *.html *.js *.mem *.data *.wasm ../media/openttd.256.png ../os/emscripten/openttd.webapp /var/www/html/
