include config.mak

APP = fosread
SRCS = fosread.c

CFLAGS += -Isrc
LDFLAGS += -Lsrc -lfosfat

all: libfosfat fosread

libfosfat:
	$(MAKE) -C src

fosread:
	$(CC) $(SRCS) $(CFLAGS) $(LDFLAGS) -o $(APP)

clean:
	$(MAKE) -C src clean
	rm -f $(APP)

install:
	$(MAKE) -C src install
	$(INSTALL) -c $(APP) $(PREFIX)/bin

install-lib:
	$(MAKE) -C src install

uninstall:
	$(MAKE) -C src uninstall
	rm -f $(PREFIX)/bin/$(APP)

.phony: clean
