#!/bin/bash

BUILD_TYPE=Release
[ "$1" = "debug" ] && BUILD_TYPE=Debug
[ "$1" = "release" ] && BUILD_TYPE=Release

OPT="-O3 -flto=thin"
[ "$BUILD_TYPE" = "Debug" ] && OPT="-g -O1"

INSTALL_PATH=/var/www/html/
[ -n "$2" ] && INSTALL_PATH="$2"

cd ../..

[ -z "`which emsdk`" ] && export PATH=`pwd`/../emsdk:$PATH

[ -z "`which emsdk`" ] && { echo "Put emsdk into your PATH"; exit 1 ; }

[ -z "$PATH_EMSDK" ] && PATH_EMSDK="`which emsdk | xargs dirname`"

source "$PATH_EMSDK/emsdk_env.sh"

mkdir -p build-wasm-$BUILD_TYPE
cd build-wasm-$BUILD_TYPE

# ===== Build dependency libraries =====

embuilder build ogg vorbis zlib sdl2 freetype icu harfbuzz
embuilder build --lto ogg vorbis zlib sdl2 freetype icu harfbuzz

autoreconf -V || exit 1 # No autotools installed

[ -e icu-le-hb-1.0.3/build-wasm/lib/libicu-le-hb.a ] || {
	wget -nc https://github.com/harfbuzz/icu-le-hb/archive/refs/tags/1.0.3.tar.gz || exit 1
	tar xvf 1.0.3.tar.gz || exit 1
	cd icu-le-hb-1.0.3
	autoreconf -fi
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		HARFBUZZ_CFLAGS="-I`em-config EMSCRIPTEN_ROOT`/cache/sysroot/include/harfbuzz -sUSE_HARFBUZZ=1" \
		HARFBUZZ_LIBS="-lharfbuzz -sUSE_HARFBUZZ=1" \
		ICU_CFLAGS="-I`em-config EMSCRIPTEN_ROOT`/cache/sysroot/include -DU_DEFINE_FALSE_AND_TRUE=1" \
		ICU_LIBS="-licuuc" \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		|| exit 1
	emmake make -j8 V=1 || exit 1
	emmake make install || exit 1
	cd ..
}

# ===== Compile libicu twice, because libiculx requires icu-le-hb, which requires libicuuc  =====
# Same libicu version as in emscripten ports and with the same compilation flags taken from $PATH_EMSDK/upstream/emscripten/tools/ports/icu.py
# We cannot use zip archive from emscripten ports, because it has Windows line endings in configure script and it won't run
#		CFLAGS="-DU_USING_ICU_NAMESPACE=0 \
#				-DU_NO_DEFAULT_INCLUDE_UTF_HEADERS=1 \
#				-DUNISTR_FROM_CHAR_EXPLICIT=explicit \
#				-DUNISTR_FROM_STRING_EXPLICIT=explicit \
#				-DU_STATIC_IMPLEMENTATION"
[ -e icu/build-wasm/lib/libiculx.a ] || {
	wget -nc https://github.com/unicode-org/icu/releases/download/release-68-2/icu4c-68_2-src.tgz || exit 1
	tar xvf icu4c-68_2-src.tgz || exit 1
	cd icu/source || exit 1

	autoreconf -fi

	# Cross-compile host tools
	[ -d cross ] || {
		mkdir -p cross
		cd cross
		../configure || exit 1
		make -j8 || exit 1
		cd ..
	}

	emconfigure ./configure --prefix=`pwd`/../build-wasm \
		--disable-shared --enable-static \
		--enable-layoutex --disable-extras \
		--disable-tools --disable-tests --disable-samples \
		--with-cross-build=`pwd`/cross \
		--with-data-packaging=archive \
		--datadir=/usr/share \
		ICULEHB_CFLAGS="-I`pwd`/../../icu-le-hb-1.0.3/build-wasm/include/icu-le-hb" \
		ICULEHB_LIBS="-L`pwd`/../../icu-le-hb-1.0.3/build-wasm/lib -licu-le-hb" \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		|| exit 1

	emmake make -j8 || exit 1

	sed -i "s@^datadir *= *.*@datadir = `pwd`/../build-wasm/share@" icudefs.mk || exit 1

	emmake make install || exit 1
	cd ../..
}

[ -e libtimidity-0.2.7/build-wasm/lib/libtimidity.a ] || {
	wget -nc https://sourceforge.net/projects/libtimidity/files/libtimidity/0.2.7/libtimidity-0.2.7.tar.gz || exit 1
	tar xvf libtimidity-0.2.7.tar.gz || exit 1
	cd libtimidity-0.2.7 || exit 1
	autoreconf -fi
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		--disable-ao --disable-aotest \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		--with-timidity-cfg="/timidity/timidity.cfg" || exit 1
	emmake make -j8 || exit 1
	emmake make install || exit 1
	cd ..
}

[ -e expat-2.3.0/build-wasm/lib/libexpat.a ] || {
	wget -nc https://github.com/libexpat/libexpat/releases/download/R_2_3_0/expat-2.3.0.tar.gz || exit 1
	tar xvf expat-2.3.0.tar.gz || exit 1
	cd expat-2.3.0
	autoreconf -fi
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		--without-xmlwf --without-docbook --without-examples --without-tests \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		|| exit 1
	emmake make -j8 || exit 1
	emmake make install || exit 1
	cd ..
}

[ -e libuuid-1.0.3/build-wasm/lib/libuuid.a ] || {
	wget -nc https://sourceforge.net/projects/libuuid/files/libuuid-1.0.3.tar.gz || exit 1
	tar xvf libuuid-1.0.3.tar.gz || exit 1
	cd libuuid-1.0.3
	autoreconf -fi
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		|| exit 1
	emmake make -j8 || exit 1
	emmake make install || exit 1
	cd ..
}

# ===== Fontconfig is convoluted shit =====

[ -e fontconfig-2.13.1/build-wasm/lib/libfontconfig.a ] || {
	wget -nc https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.13.1.tar.gz || exit 1
	tar xvf fontconfig-2.13.1.tar.gz || exit 1
	cd fontconfig-2.13.1
	# Tests do not compile, disable them
	#sed -i 's/po-conf test/po-conf/' Makefile.am
	patch -p1 < ../../os/emscripten/fontconfig-2.13.1.patch || exit 1
	autoreconf -fi
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		--disable-docs cross_compiling=yes \
		ac_cv_func_fstatfs=no ac_cv_func_fstatvfs=no \
		FREETYPE_CFLAGS="-I`em-config EMSCRIPTEN_ROOT`/cache/sysroot/include/freetype2 \
			-I`em-config EMSCRIPTEN_ROOT`/cache/sysroot/include/freetype2/freetype \
			-sUSE_FREETYPE=1" \
		FREETYPE_LIBS="-lfreetype -sUSE_FREETYPE=1" \
		EXPAT_CFLAGS="-I`pwd`/../expat-2.3.0/build-wasm/include" \
		EXPAT_LIBS="-L`pwd`/../expat-2.3.0/build-wasm/lib -lexpat" \
		UUID_CFLAGS="-I`pwd`/../libuuid-1.0.3/build-wasm/include" \
		UUID_LIBS="-L`pwd`/../libuuid-1.0.3/build-wasm/lib -luuid" \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		|| exit 1
	emmake make -j8 V=1 || exit 1
	emmake make install || exit 1
	cd ..
}

[ -e lzo-2.10/build-wasm/lib/liblzo2.a ] || {
	wget -nc https://www.oberhumer.com/opensource/lzo/download/lzo-2.10.tar.gz || exit 1
	tar xvf lzo-2.10.tar.gz || exit 1
	cd lzo-2.10
	autoreconf -fi
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		--disable-asm \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT -s EXPORTED_FUNCTIONS=['_main','_memchr','_calloc']" \
		|| exit 1
	emmake make -j8 || exit 1
	emmake make install || exit 1
	cd ..
}

[ -e xz-5.2.5/build-wasm/lib/liblzma.a ] || {
	wget -nc https://tukaani.org/xz/xz-5.2.5.tar.gz || exit 1
	tar xvf xz-5.2.5.tar.gz || exit 1
	cd xz-5.2.5
	autoreconf -fi
	emconfigure ./configure --prefix=`pwd`/build-wasm \
		--disable-shared --enable-static \
		--disable-asm \
		CFLAGS="$OPT" \
		LDFLAGS="$OPT" \
		|| exit 1
	emmake make -j8 || exit 1
	emmake make install || exit 1
	cd ..
}

# ===== Download data files =====

mkdir -p baseset

[ -e baseset/opengfx-7.1.tar ] || {
	wget -nc https://cdn.openttd.org/opengfx-releases/7.1/opengfx-7.1-all.zip || exit 1
	unzip opengfx-7.1-all.zip || exit 1
	rm opengfx-7.1-all.zip
	mv opengfx-7.1.tar baseset/ || exit 1
}

[ -e baseset/opensfx-1.0.3.tar ] || {
	wget -nc https://cdn.openttd.org/opensfx-releases/1.0.3/opensfx-1.0.3-all.zip || exit 1
	unzip opensfx-1.0.3-all.zip || exit 1
	rm opensfx-1.0.3-all.zip
	mv opensfx-1.0.3.tar baseset/ || exit 1
}

[ -e baseset/openmsx-0.4.2 ] || {
	wget -nc https://cdn.openttd.org/openmsx-releases/0.4.2/openmsx-0.4.2-all.zip || exit 1
	unzip openmsx-0.4.2-all.zip || exit 1
	rm openmsx-0.4.2-all.zip
	cd baseset || exit 1
	tar xvf ../openmsx-0.4.2.tar || exit 1
	cd ..
	rm openmsx-0.4.2.tar
}

[ -e icudt68l.dat ] || {
	cp -f "icu/source/data/in/icudt68l.dat" ./ || exit 1
}

[ -e fonts/fonts.conf ] || {
	wget -nc https://sourceforge.net/projects/libsdl-android/files/openttd-fonts.zip || exit 1
	unzip openttd-fonts.zip || exit 1
	rm openttd-fonts.zip
	sed 's@/usr/share/fonts@/fonts@g' fontconfig-2.13.1/build-wasm/etc/fonts/fonts.conf > fonts/fonts.conf || exit 1
}

#[ -e TimGM6mb.sf2 ] || {
#	wget -nc 'https://sourceforge.net/p/mscore/code/HEAD/tree/trunk/mscore/share/sound/TimGM6mb.sf2?format=raw' -O TimGM6mb.sf2 || exit 1
#}

#[ -e timidity/timidity.cfg ] || {
#	wget -nc https://sourceforge.net/projects/libsdl-android/files/timidity.zip || exit 1
#	unzip timidity.zip
#}

# ===== Build OpenTTD code generation tools =====

[ -e build-host ] || {
	rm -rf build-host
	mkdir -p build-host
	cd build-host
	cmake ../.. -DOPTION_TOOLS_ONLY=ON || exit 1
	make -j8 tools || exit 1
	cd ..
}

# ===== Build OpenTTD itself =====

export PKG_CONFIG_PATH=`pwd`/icu/build-wasm/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:`pwd`/icu-le-hb-1.0.3/build-wasm/lib/pkgconfig
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:`pwd`/../os/emscripten/cmake

[ -e Makefile ] || emcmake cmake .. \
	-DHOST_BINARY_DIR=$(pwd)/build-host -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DOPTION_USE_ASSERTS=OFF \
	-DFREETYPE_INCLUDE_DIRS=`em-config EMSCRIPTEN_ROOT`/cache/sysroot/include/freetype2 \
	-DFREETYPE_LIBRARY=-lfreetype \
	-DFontconfig_INCLUDE_DIR=`pwd`/fontconfig-2.13.1/build-wasm/include \
	-DFontconfig_LIBRARY=`pwd`/fontconfig-2.13.1/build-wasm/lib/libfontconfig.a \
	-DPC_ICU_i18n_FOUND=YES \
	-DPC_ICU_i18n_INCLUDE_DIRS=`pwd`/icu/build-wasm/include \
	-DPC_ICU_i18n_LIBRARY=`pwd`/icu/build-wasm/lib/libicui18n.a \
	-DPC_ICU_lx_FOUND=YES \
	-DPC_ICU_lx_INCLUDE_DIRS=`pwd`/icu-le-hb-1.0.3/build-wasm/include/icu-le-hb \
	-DPC_ICU_lx_LIBRARY=`pwd`/icu/build-wasm/lib/libiculx.a \
	-DLZO_INCLUDE_DIR=`pwd`/lzo-2.10/build-wasm/include \
	-DLZO_LIBRARY=`pwd`/lzo-2.10/build-wasm/lib/liblzo2.a \
	-DLIBLZMA_INCLUDE_DIR=`pwd`/xz-5.2.5/build-wasm/include \
	-DLIBLZMA_LIBRARY=`pwd`/xz-5.2.5/build-wasm/lib/liblzma.a \
	-DCMAKE_CXX_FLAGS="-sUSE_FREETYPE=1 -sUSE_HARFBUZZ=1" \
	-DWASM_LINKER_FLAGS="-sUSE_FREETYPE=1 -sUSE_HARFBUZZ=1 \
		-L`pwd`/icu-le-hb-1.0.3/build-wasm/lib -licu-le-hb -lharfbuzz \
		-L`pwd`/icu/build-wasm/lib -licudata -licuuc \
		-L`pwd`/expat-2.3.0/build-wasm/lib -lexpat \
		`pwd`/libuuid-1.0.3/build-wasm/lib/libuuid.a \
		" \
	|| exit 1

#		-L`pwd`/libuuid-1.0.3/build-wasm/lib -luuid" \


#  -I`em-config EMSCRIPTEN_ROOT`/cache/sysroot/include/freetype2/freetype
#	-DTimidity_LIBRARY=`pwd`/libtimidity-0.2.7/build-wasm/lib/libtimidity.a \
#	-DTimidity_INCLUDE_DIR=`pwd`/libtimidity-0.2.7/build-wasm/include \

[ -z "$NO_CLEAN" ] && rm -f openttd.html
emmake make -j8 VERBOSE=1 || exit 1

cp -f *.html *.js *.data *.wasm ../media/openttd.256.png ../os/emscripten/openttd.webapp "$INSTALL_PATH"
