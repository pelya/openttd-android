#!/bin/bash

BUILD_TYPE=Release
[ "$1" = "debug" ] && BUILD_TYPE=Debug
[ "$1" = "release" ] && BUILD_TYPE=Release

INSTALL_PATH=/var/www/html/
[ -n "$2" ] && INSTALL_PATH="$2"

cd ../..

[ -z "`which emsdk`" ] && export PATH=`pwd`/../emsdk:$PATH

[ -z "`which emsdk`" ] && { echo "Put emsdk into your PATH"; exit 1 ; }

[ -z "$PATH_EMSDK" ] && PATH_EMSDK="`which emsdk | xargs dirname`"

source "$PATH_EMSDK/emsdk_env.sh"

mkdir -p build-wasm-$BUILD_TYPE
cd build-wasm-$BUILD_TYPE

embuilder build liblzma ogg vorbis zlib sdl2 freetype icu harfbuzz
embuilder build --lto liblzma ogg vorbis zlib sdl2 freetype icu harfbuzz

[ -e libtimidity-0.2.7/build-wasm/lib/libtimidity.a ] || {
	wget https://sourceforge.net/projects/libtimidity/files/libtimidity/0.2.7/libtimidity-0.2.7.tar.gz || exit 1
	tar xvf libtimidity-0.2.7.tar.gz || exit 1
	cd libtimidity-0.2.7 || exit 1
	autoreconf -fi
	OPT="-O3 -flto=thin"
	[ "$BUILD_TYPE" = "Debug" ] && OPT="-g"
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		--disable-ao --disable-aotest \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		--with-timidity-cfg="/timidity/timidity.cfg" || exit 1
	make -j8 || exit 1
	make install || exit 1
	cd ..
}

[ -e expat-2.3.0/build-wasm/lib/libexpat.a ] || {
	wget https://github.com/libexpat/libexpat/releases/download/R_2_3_0/expat-2.3.0.tar.gz || exit 1
	tar xvf expat-2.3.0.tar.gz || exit 1
	cd expat-2.3.0
	autoreconf -fi
	OPT="-O3 -flto=thin"
	[ "$BUILD_TYPE" = "Debug" ] && OPT="-g"
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		--without-xmlwf --without-docbook --without-examples --without-tests \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		|| exit 1
	make -j8 || exit 1
	make install || exit 1
	cd ..
}

[ -e libuuid-1.0.3/build-wasm/lib/libuuid.a ] || {
	wget https://sourceforge.net/projects/libuuid/files/libuuid-1.0.3.tar.gz || exit 1
	tar xvf libuuid-1.0.3.tar.gz || exit 1
	cd libuuid-1.0.3
	autoreconf -fi
	OPT="-O3 -flto=thin"
	[ "$BUILD_TYPE" = "Debug" ] && OPT="-g"
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		|| exit 1
	make -j8 || exit 1
	make install || exit 1
	cd ..
}

[ -e fontconfig-2.13.1/build-wasm/lib/libfontconfig.a ] || {
	wget https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.13.1.tar.gz || exit 1
	tar xvf fontconfig-2.13.1.tar.gz || exit 1
	cd fontconfig-2.13.1
	autoreconf -fi
	OPT="-O3 -flto=thin"
	[ "$BUILD_TYPE" = "Debug" ] && OPT="-g"
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		FREETYPE_CFLAGS="-I`em-config EMSCRIPTEN_ROOT`/cache/sysroot/include/freetype2 \
			-I`em-config EMSCRIPTEN_ROOT`/cache/sysroot/include/freetype2/freetype" \
		FREETYPE_LIBS="-lfreetype" \
		EXPAT_CFLAGS="-I`pwd`/../expat-2.3.0/build-wasm/include" \
		EXPAT_LIBS="`pwd`/../expat-2.3.0/build-wasm/lib/libexpat.a" \
		UUID_CFLAGS="-I`pwd`/../libuuid-1.0.3/build-wasm/include" \
		UUID_LIBS="`pwd`/../libuuid-1.0.3/build-wasm/lib/libuuid.a" \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		|| exit 1
	make -j8 || exit 1
	make install || exit 1
	cd ..
}

[ -e build-host ] || {
	rm -rf build-host
	mkdir -p build-host
	cd build-host
	cmake ../.. -DOPTION_TOOLS_ONLY=ON || exit 1
	make -j8 tools || exit 1
	cd ..
}

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

[ -e baseset/openmsx-0.4.0 ] || {
	wget https://cdn.openttd.org/openmsx-releases/0.4.0/openmsx-0.4.0-all.zip || exit 1
	unzip openmsx-0.4.0-all.zip || exit 1
	rm openmsx-0.4.0-all.zip
	cd baseset || exit 1
	tar xvf ../openmsx-0.4.0.tar || exit 1
	cd ..
	rm openmsx-0.4.0.tar
}

[ -e icudt68l.dat ] || {
	cp -f "`em-config EMSCRIPTEN_ROOT`/cache/ports-builds/icu/source/data/in/icudt68l.dat" ./ || exit 1
}

[ -e fonts ] || {
	wget https://sourceforge.net/projects/libsdl-android/files/openttd-fonts.zip || exit 1
	unzip openttd-fonts.zip || exit 1
	rm openttd-fonts.zip
}

#[ -e TimGM6mb.sf2 ] || {
#	wget 'https://sourceforge.net/p/mscore/code/HEAD/tree/trunk/mscore/share/sound/TimGM6mb.sf2?format=raw' -O TimGM6mb.sf2 || exit 1
#}

[ -e timidity/timidity.cfg ] || {
	wget https://sourceforge.net/projects/libsdl-android/files/timidity.zip || exit 1
	unzip timidity.zip
}

[ -e Makefile ] || emcmake cmake .. -DHOST_BINARY_DIR=$(pwd)/build-host -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DOPTION_USE_ASSERTS=OFF \
									-DTimidity_LIBRARY=`pwd`/libtimidity-0.2.7/build-wasm/lib/libtimidity.a \
									-DTimidity_INCLUDE_DIR=`pwd`/libtimidity-0.2.7/build-wasm/include \
									 || exit 1

[ -z "$NO_CLEAN" ] && rm -f openttd.html
emmake make -j8 VERBOSE=1 || exit 1

cp -f *.html *.js *.data *.wasm ../media/openttd.256.png ../os/emscripten/openttd.webapp "$INSTALL_PATH"
