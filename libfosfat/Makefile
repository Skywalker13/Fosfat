include ../config.mak
include ../VERSION

ifeq ($(BUILD_STATIC),yes)
  BUILD_RULES += lib_static
endif
ifeq ($(BUILD_SHARED),yes)
  BUILD_RULES += lib_shared
endif

LIBNAME = libfosfat
STATIC_LIBNAME = ${LIBNAME}.a
SHARED_LIBNAME = ${LIBNAME}.so.${VERSION}

SRCS = fosfat.c

OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

all: depend $(BUILD_RULES)

.c.o:
	$(CC) -c $(CFLAGS) $(OPTFLAGS) -o $@ $<

lib_static: $(OBJS)
	$(AR) r $(STATIC_LIBNAME) $(OBJS)
	$(RANLIB) $(STATIC_LIBNAME)

lib_shared: $(OBJS)
	$(CC) -shared -Wl,-soname,$(SHARED_LIBNAME) \
	$(OBJS) $(LDFLAGS) $(EXTRALIBS) -o $(SHARED_LIBNAME) 

clean:
	rm -f *.o *.a *.so*
	rm -f .depend

install: $(BUILD_RULES)
	[ $(BUILD_STATIC) = yes ] && $(INSTALL) -c $(STATIC_LIBNAME) $(libdir) || continue
	[ $(BUILD_SHARED) = yes ] && $(INSTALL) -c $(SHARED_LIBNAME) $(libdir) || continue
	$(INSTALL) -c -m 644 fosfat.h $(includedir)

uninstall:
	rm -f $(libdir)/$(STATIC_LIBNAME)
	rm -f $(libdir)/$(SHARED_LIBNAME)
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
