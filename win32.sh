#!/bin/sh

. ./VERSION

touch config.mak

CONFIGFILE="config.win32"

cc="i586-mingw32msvc-gcc"
cxx="i586-mingw32msvc-g++"

cflags="-Wall -Wextra -Werror -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -O3"
ldflags=

append_config(){
    echo "$@" >> $CONFIGFILE
}


echo "# Automatically generated - do not modify!" > $CONFIGFILE

append_config "CC=$cc"
append_config "CXX=$cxx"
append_config "CFLAGS=$cflags"
append_config "LDFLAGS=$ldflags"
