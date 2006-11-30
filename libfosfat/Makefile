include ../config.mak

LIBNAME = libfosfat.so

SRCS = fosfat.c

CFLAGS += -fPIC

OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

all: depend $(LIBNAME)

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

$(LIBNAME): $(OBJS)
	$(CC) -shared $(OBJS) $(LDFLAGS) -o $(LIBNAME)

clean:
	rm -f *.o *.a *.so
	rm -f .depend

install: $(LIBNAME)
	$(INSTALL) -c $(LIBNAME) $(PREFIX)/lib
	$(INSTALL) -c -m 644 fosfat.h $(PREFIX)/include

uninstall: $(LIBNAME)
	rm -f $(PREFIX)/lib/$(LIBNAME)
	rm -f $(PREFIX)/include/fosfat.h

depend:
	$(CC) -MM $(CFLAGS) $(SRCS) 1>.depend

.phony: clean depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
