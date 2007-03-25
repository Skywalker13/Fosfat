#!/bin/sh

. ./VERSION

DIR=fosfat-$VERSION.$PATCHLEVEL.$SUBLEVEL
PACKAGE=package-deb

rm -rf "$PACKAGE"
mkdir -p "$PACKAGE/$DIR"

touch config.mak
make distclean

cp -pPR Makefile README COPYING VERSION TODO ChangeLog package-deb.sh configure fosmount tools libfosfat debian "$PACKAGE/$DIR"
find "$PACKAGE/$DIR" \( -name .svn -or -name .depend -or -name '*.o' \) -exec rm -rf '{}' \; 2>/dev/null
cd "$PACKAGE"
tar -czf "fosfat_$VERSION.$PATCHLEVEL.$SUBLEVEL.orig.tar.gz" "$DIR"

cd "$DIR"
debuild -S -uc

cd ..
rm -rf "$DIR"
