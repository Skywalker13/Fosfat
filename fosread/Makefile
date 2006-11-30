include ../config.mak

APPNAME = fosread
SRCS = fosread.c

CFLAGS += -I../libfosfat
LDFLAGS += -L../libfosfat -lfosfat

OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

all: depend $(APPNAME)

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(APPNAME): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(APPNAME)

clean:
	rm -f *.o $(APPNAME)
	rm -f .depend

install: $(APPNAME)
	$(INSTALL) -c $(APPNAME) $(PREFIX)/bin

uninstall: $(APPNAME)
	rm -f $(PREFIX)/bin/$(APPNAME)

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

.phony: clean depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
