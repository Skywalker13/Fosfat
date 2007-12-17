ifeq (,$(wildcard ../config.win32))
$(error "../config.win32 is not present, run `make win32` !")
endif
include ../config.win32

BUILD_RULES += lib_dll
CFLAGS += -DBUILD_DLL

LIBNAME = w32disk
STATIC_LIBNAME = lib${LIBNAME}dll.a
SHARED_LIBNAME = ${LIBNAME}.dll

SRCS = Disk.cxx wrapper.cxx

OBJS = $(SRCS:.cxx=.o)

.SUFFIXES: .cxx .o

all: depend $(BUILD_RULES)

.cxx.o:
	$(CXX) -c $(CFLAGS) $(OPTFLAGS) -o $@ $<

lib_dll: $(OBJS)
	$(CXX) $(CFLAGS) $(OPTFLAGS) -shared -o $(SHARED_LIBNAME) -Wl,--out-implib,$(STATIC_LIBNAME) $(OBJS) $(LDFLAGS)
	$(STRIP) $(SHARED_LIBNAME)

clean:
	rm -f *.o *.a *.dll
	rm -f .depend

depend:
	$(CXX) -MM $(CFLAGS) $(OPTFLAGS) $(SRCS) 1>.depend

.phony: clean depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
