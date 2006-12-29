include ../config.mak

LIBNAME = libfosfat.a

SRCS = fosfat.c

CFLAGS += -fPIC

OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

all: depend $(LIBNAME)

.c.o:
	$(CC) -c $(CFLAGS) $(OPTFLAGS) -o $@ $<

$(LIBNAME): $(OBJS)
	$(AR) r $(LIBNAME) $(OBJS)
	$(RANLIB) $(LIBNAME)

clean:
	rm -f *.o *.a *.so
	rm -f .depend

install: $(LIBNAME)
	$(INSTALL) -c $(LIBNAME) $(libdir)
	$(INSTALL) -c -m 644 fosfat.h $(includedir)

uninstall: $(LIBNAME)
	rm -f $(libdir)/$(LIBNAME)
	rm -f $(includedir)/fosfat.h

depend:
	$(CC) -MM $(CFLAGS) $(OPTFLAGS) $(SRCS) 1>.depend

.phony: clean depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
