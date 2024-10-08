ifeq (,$(wildcard ../config.mak))
$(error "../config.mak is not present, run configure !")
endif
include ../config.mak

FOSMOUNT = fosmount
FOSMOUNT_SRCS = fosmount.c
FOSMOUTN_MAN = $(FOSMOUNT).1

APPS_CPPFLAGS = -I../libfosfat -I../libfosgra $(CFG_CPPFLAGS) $(CPPFLAGS)
APPS_LDFLAGS = -L../libfosfat -L../libfosgra -lfosfat -lfosgra -lfuse3 $(CFG_LDFLAGS) $(LDFLAGS)

MANS = $(FOSMOUTN_MAN)

EXTRADIST = \
	$(MANS)

OBJS = $(FOSMOUNT_SRCS:.c=.o)

ifeq ($(BUILD_STATIC),yes)
ifeq ($(BUILD_SHARED),no)
  APPS_LDFLAGS += $(EXTRALIBS)
endif
endif

.SUFFIXES: .c .o

all: app

.c.o:
	$(CC) -c $(OPTFLAGS) $(CFLAGS) $(APPS_CPPFLAGS) -o $@ $<

$(FOSMOUNT): $(OBJS)
	$(CC) $(OBJS) $(APPS_LDFLAGS) -o $(FOSMOUNT)

app-dep:
	$(CC) -MM $(CFLAGS) $(APPS_CPPFLAGS) $(FOSMOUNT_SRCS) 1>.depend

app: app-dep
	$(MAKE) $(FOSMOUNT)

clean:
	rm -f *.o $(FOSMOUNT)
	rm -f .depend

install: install-app install-man

install-app: app
	$(INSTALL) -d $(bindir)
	$(INSTALL) -c -m 755 $(FOSMOUNT) $(bindir)

install-man: $(MANS)
	for m in $(MANS); do \
	  section=`echo $$m | sed -e 's/^.*\\.//'`; \
	  $(INSTALL) -d $(mandir)/man$$section; \
	  $(INSTALL) -m 644 $$m $(mandir)/man$$section; \
	done

uninstall: uninstall-app uninstall-man

uninstall-app:
	rm -f $(bindir)/$(FOSMOUNT)

uninstall-man:
	for m in $(MANS); do \
	  section=`echo $$m | sed -e 's/^.*\\.//'`; \
	  rm -f $(mandir)/man$$section/$$m; \
	done

.PHONY: *clean *install* app*

dist-all:
	cp $(EXTRADIST) $(FOSMOUNT_SRCS) Makefile $(DIST)

.PHONY: dist dist-all

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
