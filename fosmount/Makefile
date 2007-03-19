include ../config.mak
include ../VERSION

TARGETI =
TARGETB =
APPNAME = fosmount
SRCS = fosmount.c

CFLAGS += -I../libfosfat -DVERSION=\"$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRA)\"
LDFLAGS += -L../libfosfat -lfosfat

OBJS = $(SRCS:.c=.o)

ifeq ($(FOSMOUNT),yes)
TARGETI = install-app
TARGETB = depend $(APPNAME)
endif

.SUFFIXES: .c .o

all: $(TARGETB)

.c.o:
	$(CC) -c $(CFLAGS) $(OPTFLAGS) -o $@ $<

$(APPNAME): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(EXTRALIBS) -o $(APPNAME)

clean:
	rm -f *.o $(APPNAME)
	rm -f .depend

install: $(TARGETI)

install-app: $(APPNAME)
	$(INSTALL) -c -m 755 $(APPNAME) $(bindir)

uninstall:
	rm -f $(bindir)/$(APPNAME)

depend:
	$(CC) -MM $(CFLAGS) $(OPTFLAGS) $(SRCS) 1>.depend

.phony: clean depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
