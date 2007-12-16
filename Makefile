ifeq (,$(wildcard config.mak))
MAKE=make
else
include config.mak
endif

all:
	cp -f libfosfat/Makefile.linux libfosfat/Makefile
	cp -f tools/Makefile.linux tools/Makefile
	$(MAKE) -C libfosfat
	$(MAKE) -C tools
	$(MAKE) -C fosmount

clean:
	$(MAKE) -C libw32disk clean
	$(MAKE) -C libfosfat clean
	$(MAKE) -C tools clean
	$(MAKE) -C fosmount clean

distclean: clean
	rm -f config.log
	rm -f config.mak
	rm -f config.win32
	rm -f libfosfat/Makefile
	rm -f tools/Makefile

install: install-deb install-dev

install-deb:
	$(MAKE) -C tools install
	$(MAKE) -C fosmount install

install-dev:
	$(MAKE) -C libfosfat install

uninstall:
	$(MAKE) -C libfosfat uninstall
	$(MAKE) -C tools uninstall
	$(MAKE) -C fosmount uninstall

win32-dev:
	./win32-gen.sh

win32-build:
	./win32-gen.sh build

win32-common:
	cp -f libfosfat/Makefile.win32 libfosfat/Makefile
	cp -f tools/Makefile.win32 tools/Makefile
	$(MAKE) -C libw32disk
	$(MAKE) -C libfosfat
	$(MAKE) -C tools

win32: win32-dev win32-common

win32-zip: win32-build win32-common

.phony: clean distclean
