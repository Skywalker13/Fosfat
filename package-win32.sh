#!/bin/sh

. ./VERSION

make distclean
make win32-zip

DIR=fosfat-$VERSION.$PATCHLEVEL.$SUBLEVEL
PACKAGE=package-win32

rm -rf "$PACKAGE"
mkdir -p "$PACKAGE/$DIR"

cp -pP README \
       TODO \
       COPYING \
       libfosfat/fosfat.dll \
       libw32disk/w32disk.dll \
       tools/fosread.exe \
       tools/smascii.exe \
       "$PACKAGE/$DIR"

cd "$PACKAGE"
zip -r -9 "fosfat_$VERSION.$PATCHLEVEL.$SUBLEVEL.win32.zip" "$DIR"

rm -rf "$DIR"
cd ..

make distclean
