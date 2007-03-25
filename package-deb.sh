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
        fosmount \
        tools \
        libfosfat \
        "$PACKAGE/$DIR"
find "$PACKAGE/$DIR" \( -name .svn -or \
                        -name .depend* -or \
                        -name '*.o' -or \
                        -name '*.a' \
                     \) -exec rm -rf '{}' \; 2>/dev/null
find "$PACKAGE/$DIR" \( -name 'fosread' -or \
                        -name 'smascii' -or \
                        -name 'fosmount' \
                     \) -exec rm -f '{}' \; 2>/dev/null

echo "cd $PACKAGE && sudo pbuilder build *dsc" 1> gen-deb.sh
chmod a+x gen-deb.sh
cd "$PACKAGE"
tar -czf "fosfat_$VERSION.$PATCHLEVEL.$SUBLEVEL.orig.tar.gz" "$DIR"

cp -pPR ../debian "$DIR"
find "$DIR" \( -name .svn \) -exec rm -rf '{}' \; 2>/dev/null
cd "$DIR"
debuild -S -sa

cd ..
rm -rf "$DIR"
