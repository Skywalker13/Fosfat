include config.mak

all:
	$(MAKE) -C libfosfat
	$(MAKE) -C tools
	$(MAKE) -C fosmount

clean:
	$(MAKE) -C libfosfat clean
	$(MAKE) -C tools clean
	$(MAKE) -C fosmount clean

distclean: clean
	rm -f config.log
	rm -f config.mak

install:
	$(MAKE) -C tools install
	$(MAKE) -C fosmount install

install-lib:
	$(MAKE) -C libfosfat install

uninstall:
	$(MAKE) -C libfosfat uninstall
	$(MAKE) -C tools uninstall
	$(MAKE) -C fosmount uninstall

.phony: clean distclean
