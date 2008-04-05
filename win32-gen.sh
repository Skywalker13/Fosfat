#!/bin/sh

. ./VERSION

[ "$1" = build ] && EXTRA=""

touch config.mak

CONFIGFILE="config.win32"

cross="i586-mingw32msvc-"
cc="${cross}gcc"
cxx="${cross}g++"
strip="${cross}strip"

cflags="-Wall -Wextra -Werror -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -O3 -DVERSION=\"\\\"$VERSION.$PATCHLEVEL.$SUBLEVEL$EXTRA\\\"\""
ldflags=

append_config(){
    echo "$@" >> $CONFIGFILE
}


echo "# Automatically generated - do not modify!" > $CONFIGFILE

append_config "CC=$cc"
append_config "CXX=$cxx"
append_config "STRIP=$strip"
append_config "CFLAGS=$cflags"
append_config "LDFLAGS=$ldflags"
