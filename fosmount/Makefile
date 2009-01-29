ifeq (,$(wildcard ../config.mak))
$(error "../config.mak is not present, run configure !")
endif
include ../config.mak
include ../VERSION

TARGETI =
TARGETB =
APPNAME = fosmount
SRCS = fosmount.c
MANPAGES = fosmount.1

OPTFLAGS += -I../libfosfat -DVERSION=\"$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRA)\"
OPTLDFLAGS = -L../libfosfat -lfosfat

ifeq ($(FOSMOUNT),yes)
TARGETI = install-app install-man
TARGETB = $(APPNAME)
endif

all: $(TARGETB)

$(APPNAME):
	$(CC) $(SRCS) $(CFLAGS) $(OPTFLAGS) $(LDFLAGS) $(OPTLDFLAGS) $(EXTRALIBS) -o $(APPNAME)

clean:
	rm -f $(APPNAME)

install: $(TARGETI)

install-app: $(APPNAME)
	$(INSTALL) -d $(bindir)
	$(INSTALL) $(APPNAME) $(bindir)
	$(STRIP) $(INSTALLSTRIP) $(bindir)/$(APPNAME)

install-man:
	install -d $(mandir)/man1
	install -m 644 $(MANPAGES) $(mandir)/man1

uninstall: uninstall-man
	rm -f $(bindir)/$(APPNAME)

uninstall-man:
	rm -f $(mandir)/man1/$(MANPAGES)

.PHONY: clean install-man uninstall-man
