include config.mak

all: libfos read

libfos:
	$(MAKE) -C libfosfat

read:
	$(MAKE) -C fosread

clean:
	$(MAKE) -C libfosfat clean
	$(MAKE) -C fosread clean

install:
	$(MAKE) -C libfosfat install
	$(MAKE) -C fosread install

install-lib:
	$(MAKE) -C libfosfat install

uninstall:
	$(MAKE) -C libfosfat uninstall
	$(MAKE) -C fosread uninstall

.phony: clean
