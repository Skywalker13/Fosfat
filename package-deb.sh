#!/bin/sh

. ./VERSION

DIR=fosfat-$VERSION.$PATCHLEVEL.$SUBLEVEL
PACKAGE=package-deb

rm -rf "$PACKAGE"
mkdir -p "$PACKAGE/$DIR"

cp -pPR Makefile \
        README \
        COPYING \
        VERSION \
        TODO \
        ChangeLog \
        configure \
        win32-gen.sh \
        fosmount \
        tools \
        libfosfat \
        libw32disk \
        "$PACKAGE/$DIR"
find "$PACKAGE/$DIR" \( -name .svn -or \
                        -name .depend* -or \
                        -name '*.o' -or \
                        -name '*.a' -or \
                        -name '*.so*' -or \
                        -name '*.dll' -or \
                        -name '*~' \
                     \) -exec rm -rf '{}' \; 2>/dev/null
find "$PACKAGE/$DIR" \( -name 'fosread' -or \
                        -name 'smascii' -or \
                        -name 'fosmount' \
                     \) -exec rm -f '{}' \; 2>/dev/null

cd "$PACKAGE"
sed -i "s/EXTRA=.*/EXTRA=/" $DIR/VERSION
tar -czf "fosfat_$VERSION.$PATCHLEVEL.$SUBLEVEL.orig.tar.gz" "$DIR"

cp -pPR ../debian "$DIR"
find "$DIR" \( -name .svn -or -name '*~' \) -exec rm -rf '{}' \; 2>/dev/null
cd "$DIR"
debuild -us -uc

cd ..
rm -rf "$DIR"
