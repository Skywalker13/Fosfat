ifeq (,$(wildcard ../config.mak))
$(error "../config.mak is not present, run configure !")
endif
include ../config.mak

VERSION = "0.0.1"

LIBNAME = libw32disk
STATIC_LIBNAME = ${LIBNAME}.a
SHARED_LIBNAME = $(LIBNAME)-$(shell echo $(VERSION) | cut -f1 -d.).dll
SHARED_LIBNAME_VERSION = $(SHARED_LIBNAME)
SHARED_LIBNAME_MAJOR = $(SHARED_LIBNAME)
SHARED_LIBNAME_FLAGS = -shared -Wl,--out-implib=$(LIBNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import

ifeq ($(BUILD_STATIC),yes)
  BUILD_RULES += lib_static
endif
ifeq ($(BUILD_SHARED),yes)
  BUILD_RULES += lib_shared
endif

SRCS =  w32disk.c \

EXTRADIST = \
	w32disk.h \

OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

all: depend $(BUILD_RULES)

.cxx.o:
	$(CC) -c $(OPTFLAGS) $(CFLAGS) $(CFG_CPPFLAGS) $(CPPFLAGS) -o $@ $<

lib_static: $(STATIC_LIBNAME)

lib_shared: $(SHARED_LIBNAME)

$(STATIC_LIBNAME): $(OBJS)
	$(AR) r $(STATIC_LIBNAME) $(OBJS)
	$(RANLIB) $(STATIC_LIBNAME)

$(SHARED_LIBNAME): $(OBJS)
	$(CC) $(SHARED_LIBNAME_FLAGS) \
	  $(OBJS) $(CFG_LDFLAGS) $(EXTRALIBS) $(LDFLAGS) -o $(SHARED_LIBNAME_VERSION)

clean:
	rm -f *.o *.a *.dll
	rm -f .depend

install: $(BUILD_RULES)
	$(INSTALL) -d $(libdir)
	[ $(BUILD_STATIC) = yes ] && $(INSTALL) -c $(STATIC_LIBNAME) $(libdir); \
	if [ $(BUILD_SHARED) = yes ]; then \
	  $(INSTALL) -c $(SHARED_LIBNAME_VERSION) $(bindir); \
	  $(INSTALL) -c $(LIBNAME).dll.a $(libdir); \
	fi
	$(INSTALL) -d $(includedir)
	$(INSTALL) -c -m 644 w32disk.h $(includedir)

uninstall:
	rm -f $(bindir)/$(SHARED_LIBNAME); \
	rm -f $(libdir)/$(LIBNAME).dll.a; \
	rm -f $(libdir)/$(STATIC_LIBNAME)
	rm -f $(libdir)/$(SHARED_LIBNAME)*
	rm -f $(includedir)/fosfat.h

depend:
	$(CC) -MM $(CFLAGS) $(CFG_CPPFLAGS) $(CPPFLAGS) $(SRCS) 1>.depend

.PHONY: clean depend
.PHONY: uninstall

dist-all:
	cp $(EXTRADIST) $(SRCS) Makefile $(DIST)

.PHONY: dist-all

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
