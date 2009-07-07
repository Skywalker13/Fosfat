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

OPTFLAGS += -I../libfosfat -I../libfosgra -DVERSION=\"$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRA)\"
OPTLDFLAGS = -L../libfosfat -L../libfosgra -lfosfat -lfosgra

OBJS = $(SRCS:.c=.o)

ifeq ($(FOSMOUNT),yes)
TARGETI = install-app install-man
TARGETB = depend $(APPNAME)
endif

.SUFFIXES: .c .o

all: $(TARGETB)

.c.o:
	$(CC) -c $(CFLAGS) $(OPTFLAGS) -o $@ $<

$(APPNAME): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(OPTLDFLAGS) $(EXTRALIBS) -o $(APPNAME)

clean:
	rm -f *.o $(APPNAME)
	rm -f .depend

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

depend:
	$(CC) -MM $(CFLAGS) $(OPTFLAGS) $(SRCS) 1>.depend

.PHONY: clean install-man uninstall-man depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
