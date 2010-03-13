ifeq (,$(wildcard ../config.mak))
$(error "../config.mak is not present, run configure !")
endif
include ../config.mak

LIBNAME = libfosfat
STATIC_LIBNAME = ${LIBNAME}.a
SHARED_LIBNAME = $(LIBNAME).so
SHARED_LIBNAME_VERSION = $(SHARED_LIBNAME).$(VERSION)
SHARED_LIBNAME_MAJOR = $(SHARED_LIBNAME).$(shell echo $(VERSION) | cut -f1 -d.)
SHARED_LIBNAME_FLAGS = -shared -Wl,-soname,$(SHARED_LIBNAME_MAJOR)

ifeq ($(BUILD_STATIC),yes)
  BUILD_RULES += lib_static
endif
ifeq ($(BUILD_SHARED),yes)
  BUILD_RULES += lib_shared
  ifeq ($(BUILD_DYLIB),yes)
    SHARED_LIBNAME         = $(LIBNAME).dylib
    SHARED_LIBNAME_VERSION = $(LIBNAME).$(VERSION).dylib
    SHARED_LIBNAME_MAJOR   = $(LIBNAME).$(shell echo $(VERSION) | cut -f1 -d.).dylib
    SHARED_LIBNAME_FLAGS   = -dynamiclib -Wl,-headerpad_max_install_names,-undefined,dynamic_lookup,-install_name,$(SHARED_LIBNAME_VERSION)
  else
    ifeq ($(BUILD_MINGW32),yes)
      SHARED_LIBNAME         = $(LIBNAME)-$(shell echo $(VERSION) | cut -f1 -d.).dll
      SHARED_LIBNAME_VERSION = $(SHARED_LIBNAME)
      SHARED_LIBNAME_MAJOR   = $(SHARED_LIBNAME)
      SHARED_LIBNAME_FLAGS   = -shared -Wl,--out-implib=$(LIBNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import
    endif
  endif
endif

ifeq ($(BUILD_MINGW32),yes)
  LIB_CPPFLAGS = -I../libw32disk $(CFG_CPPFLAGS) $(CPPFLAGS)
  LIB_LDFLAGS = -L../libw32disk $(CFG_LDFLAGS) $(LDFLAGS) -lw32disk
else
  LIB_CPPFLAGS = $(CFG_CPPFLAGS) $(CPPFLAGS)
  LIB_LDFLAGS = $(CFG_LDFLAGS) $(LDFLAGS)
endif

SRCS =  fosfat.c \

EXTRADIST = \
	fosfat.h \

OBJS = $(SRCS:.c=.o)

.SUFFIXES: .c .o

all: depend $(BUILD_RULES)

.c.o:
	$(CC) -c $(OPTFLAGS) $(CFLAGS) $(LIB_CPPFLAGS) -o $@ $<

lib_static: $(STATIC_LIBNAME)

lib_shared: $(SHARED_LIBNAME)

$(STATIC_LIBNAME): $(OBJS)
	$(AR) r $(STATIC_LIBNAME) $(OBJS)
	$(RANLIB) $(STATIC_LIBNAME)

$(SHARED_LIBNAME): $(OBJS)
	$(CC) $(SHARED_LIBNAME_FLAGS) \
	  $(OBJS) $(EXTRALIBS) $(LIB_LDFLAGS) -o $(SHARED_LIBNAME_VERSION)
	if [ $(BUILD_MINGW32) = no ]; then \
	  $(LN) -sf $(SHARED_LIBNAME_VERSION) $(SHARED_LIBNAME_MAJOR); \
	  $(LN) -sf $(SHARED_LIBNAME_MAJOR) $(SHARED_LIBNAME); \
	fi

clean:
	rm -f *.o *.a *.so* *.dll
	rm -f .depend

install: $(BUILD_RULES)
	$(INSTALL) -d $(libdir)
	[ $(BUILD_STATIC) = yes ] && $(INSTALL) -c $(STATIC_LIBNAME) $(libdir); \
	if [ $(BUILD_SHARED) = yes ]; then \
	  if [ $(BUILD_MINGW32) = no ]; then \
	    $(INSTALL) -c $(SHARED_LIBNAME_VERSION) $(libdir); \
	    $(LN) -sf $(SHARED_LIBNAME_VERSION) $(libdir)/$(SHARED_LIBNAME_MAJOR); \
	    $(LN) -sf $(SHARED_LIBNAME_MAJOR) $(libdir)/$(SHARED_LIBNAME); \
	  else \
	    $(INSTALL) -c $(SHARED_LIBNAME_VERSION) $(bindir); \
	    $(INSTALL) -c $(LIBNAME).dll.a $(libdir); \
	  fi \
	fi
	$(INSTALL) -d $(includedir)
	$(INSTALL) -c -m 644 fosfat.h $(includedir)

uninstall:
	if [ $(BUILD_MINGW32) = yes ]; then \
	  rm -f $(bindir)/$(SHARED_LIBNAME); \
	  rm -f $(libdir)/$(LIBNAME).dll.a; \
	fi
	rm -f $(libdir)/$(STATIC_LIBNAME)
	rm -f $(libdir)/$(SHARED_LIBNAME)*
	rm -f $(includedir)/fosfat.h

depend:
	$(CC) -MM $(CFLAGS) $(LIB_CPPFLAGS) $(SRCS) 1>.depend

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
