ifeq (,$(wildcard ../config.mak))
$(error "../config.mak is not present, run configure !")
endif
include ../config.mak

FOSREAD = fosread
FOSREAD_SRCS = fosread.c
FOSREAD_OBJS = $(FOSREAD_SRCS:.c=.o)
FOSREAD_MAN = $(FOSREAD).1
SMASCII = smascii
SMASCII_SRCS = ascii.c smascii.c
SMASCII_OBJS = $(SMASCII_SRCS:.c=.o)
SMASCII_MAN = $(SMASCII).1
FOSREC = fosrec
FOSREC_SRCS = fosrec.c
FOSREC_OBJS = $(FOSREC_SRCS:.c=.o)
FOSREC_MAN = $(FOSREC).1

APPS_CPPFLAGS = -I../libfosfat $(CFG_CPPFLAGS) $(CPPFLAGS)
ifeq ($(BUILD_MINGW32),yes)
  STDCXX_LDFLAGS =
  ifeq ($(BUILD_STATIC),yes)
  ifeq ($(BUILD_SHARED),no)
    STDCXX_LDFLAGS = -lstdc++
  endif
  endif
  APPS_LDFLAGS = -L../libfosfat -L../libw32disk -lfosfat -lw32disk $(CFG_LDFLAGS) $(LDFLAGS) $(STDCXX_LDFLAGS)
else
  APPS_LDFLAGS = -L../libfosfat -lfosfat $(CFG_LDFLAGS) $(LDFLAGS)
endif

MANS = $(FOSREAD_MAN) $(SMASCII_MAN) $(FOSREC_MAN)

EXTRADIST = \
	$(MANS)

ifeq ($(BUILD_STATIC),yes)
ifeq ($(BUILD_SHARED),no)
  APPS_LDFLAGS += $(EXTRALIBS)
endif
endif

.SUFFIXES: .c .o

all: apps

.c.o:
	$(CC) -c $(OPTFLAGS) $(CFLAGS) $(APPS_CPPFLAGS) -o $@ $<

$(FOSREAD): $(FOSREAD_OBJS)
	$(CC) $(FOSREAD_OBJS) $(APPS_LDFLAGS) -o $(FOSREAD)
$(SMASCII): $(SMASCII_OBJS)
	$(CC) $(SMASCII_OBJS) $(APPS_LDFLAGS) -o $(SMASCII)
$(FOSREC): $(FOSREC_OBJS)
	$(CC) $(FOSREC_OBJS) $(APPS_LDFLAGS) -o $(FOSREC)

apps-dep:
	$(CC) -MM $(CFLAGS) $(APPS_CPPFLAGS) $(FOSREAD_SRCS) 1>.depend
	$(CC) -MM $(CFLAGS) $(APPS_CPPFLAGS) $(SMASCII_SRCS) 1>>.depend
	$(CC) -MM $(CFLAGS) $(APPS_CPPFLAGS) $(FOSREC_SRCS) 1>>.depend

apps-all: $(FOSREAD) $(SMASCII) $(FOSREC)

apps: apps-dep
	$(MAKE) apps-all

clean:
	rm -f *.o
	rm -f $(FOSREAD)
	rm -f $(SMASCII)
	rm -f $(FOSREC)
	rm -f .depend

install: install-apps install-man

install-apps: apps
	$(INSTALL) -d $(bindir)
	$(INSTALL) -c -m 755 $(FOSREAD) $(bindir)
	$(INSTALL) -c -m 755 $(SMASCII) $(bindir)
	$(INSTALL) -c -m 755 $(FOSREC) $(bindir)

install-man: $(MANS)
	for m in $(MANS); do \
	  section=`echo $$m | sed -e 's/^.*\\.//'`; \
	  $(INSTALL) -d $(mandir)/man$$section; \
	  $(INSTALL) -m 644 $$m $(mandir)/man$$section; \
	done

uninstall: uninstall-apps uninstall-man

uninstall-apps:
	rm -f $(bindir)/$(FOSREAD)
	rm -f $(bindir)/$(SMASCII)
	rm -f $(bindir)/$(FOSREC)

uninstall-man:
	for m in $(MANS); do \
	  section=`echo $$m | sed -e 's/^.*\\.//'`; \
	  rm -f $(mandir)/man$$section/$$m; \
	done

.PHONY: *clean *install* apps*

dist-all:
	cp $(EXTRADIST) $(FOSREAD_SRCS) $(SMASCII_SRCS) $(FOSREC_SRCS) Makefile $(DIST)

.PHONY: dist dist-all

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
