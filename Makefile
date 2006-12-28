include config.mak

all:
	$(MAKE) -C libfosfat
	$(MAKE) -C fosread
	$(MAKE) -C fosmount

clean:
	$(MAKE) -C libfosfat clean
	$(MAKE) -C fosread clean
	$(MAKE) -C fosmount clean

install:
	$(MAKE) -C fosread install
	$(MAKE) -C fosmount install

install-lib:
	$(MAKE) -C libfosfat install

uninstall:
	$(MAKE) -C libfosfat uninstall
	$(MAKE) -C fosread uninstall
	$(MAKE) -C fosmount uninstall

.phony: clean
