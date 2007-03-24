include ../config.mak
include ../VERSION

TARGETI =
TARGETB =
APPNAME1 = fosread
APPNAME2 = smascii
SRCS1 = fosread.c
SRCS2 = ascii.c smascii.c

CFLAGS1 = $(CFLAGS) -I../libfosfat -DVERSION=\"$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRA)\"
LDFLAGS1 = $(LDFLAGS) -L../libfosfat -lfosfat

CFLAGS2 = $(CFLAGS)
LDFLAGS2 = $(LDFLAGS)

OBJS1 = $(SRCS1:.c=.o)
OBJS2 = $(SRCS2:.c=.o)

ifeq ($(TOOLS),yes)
TARGETI = install-app
TARGETB = depend $(APPNAME1) $(APPNAME2)
endif

.SUFFIXES: .c .o

all: $(TARGETB)

.c.o:
	$(CC) -c $(CFLAGS1) $(OPTFLAGS) -o $@ $<

$(APPNAME1): $(OBJS1)
	$(CC) $(OBJS1) $(LDFLAGS1) $(EXTRALIBS) -o $(APPNAME1)

$(APPNAME2): $(OBJS2)
	$(CC) $(OBJS2) $(LDFLAGS2) -o $(APPNAME2)

clean:
	rm -f *.o $(APPNAME1) $(APPNAME2)
	rm -f .depend*

install: $(TARGETI)

install-app: $(APPNAME1) $(APPNAME2)
	$(INSTALL) -c -m 755 $(APPNAME1) $(bindir)
	$(INSTALL) -c -m 755 $(APPNAME2) $(bindir)

uninstall:
	rm -f $(bindir)/$(APPNAME1)
	rm -f $(bindir)/$(APPNAME2)

depend:
	$(CC) -MM $(CFLAGS1) $(OPTFLAGS) $(SRCS1) 1>.depend1
	$(CC) -MM $(CFLAGS2) $(OPTFLAGS) $(SRCS2) 1>.depend2

.phony: clean depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend1 .depend2),)
include .depend1
include .depend2
endif
