#!/bin/sh

. ./version

DIR=fosfat-$VERSION.$PATCHLEVEL.$SUBLEVEL
PACKAGE=package-deb

rm -rf "$PACKAGE"
mkdir -p "$PACKAGE/$DIR"

cp -pPR Makefile README COPYING version ChangeLog package-deb.sh configure fosmount fosread libfosfat debian "$PACKAGE/$DIR"
find "$PACKAGE/$DIR" \( -name .svn -or -name .depend -or -name '*.o' \) -exec rm -rf '{}' \; 2>/dev/null
cd "$PACKAGE"
tar -czf "fosfat_$VERSION.$PATCHLEVEL.$SUBLEVEL.orig.tar.gz" "$DIR"

cd "$DIR"
debuild -us -uc

rm -rf "$DIR"
