ifeq (,$(wildcard config.mak))
$(error "config.mak is not present, run configure !")
endif
include config.mak

PKGCONFIG_DIR = $(libdir)/pkgconfig
PKGCONFIG_FILE = libfosfat.pc \
		 libfosgra.pc \

DISTFILE = fosfat-$(VERSION).tar.bz2

EXTRADIST = \
	ChangeLog \
	configure \
	COPYING \
	README \
	TODO \

SUBDIRS = \
	DOCS \
	fosmount \
	libfosfat \
	libfosgra \
	libw32disk \
	tools \

all: libs fosmount tools docs

config.mak: configure
	@echo "############################################################"
	@echo "####### Please run ./configure again - it's changed! #######"
	@echo "############################################################"

libs:
ifeq ($(BUILD_MINGW32),yes)
	$(MAKE) -C libw32disk
endif
	$(MAKE) -C libfosfat
	$(MAKE) -C libfosgra

fosmount: libs
ifeq ($(FOSMOUNT),yes)
	$(MAKE) -C fosmount
endif

tools: libs
ifeq ($(TOOLS),yes)
	$(MAKE) -C tools
endif

docs:
	$(MAKE) -C DOCS

docs-clean:
	$(MAKE) -C DOCS clean

clean:
	$(MAKE) -C fosmount clean
	$(MAKE) -C libfosfat clean
	$(MAKE) -C libfosgra clean
	$(MAKE) -C libw32disk clean
	$(MAKE) -C tools clean

distclean: clean docs-clean
	rm -f config.log
	rm -f config.mak
	rm -f $(DISTFILE)
	rm -f $(PKGCONFIG_FILE)

install: install-libs install-pkgconfig install-fosmount install-tools install-docs

install-libs:
	$(MAKE) -C libfosfat install
	$(MAKE) -C libfosgra install

install-pkgconfig: $(PKGCONFIG_FILE)
	$(INSTALL) -d "$(PKGCONFIG_DIR)"
	$(INSTALL) -m 644 $< "$(PKGCONFIG_DIR)"

install-fosmount: fosmount
	$(MAKE) -C fosmount install

install-tools: tools
	$(MAKE) -C tools install

install-docs: docs
	$(MAKE) -C DOCS install

uninstall: uninstall-libs uninstall-pkgconfig uninstall-fosmount uninstall-tools uninstall-docs

uninstall-libs:
	$(MAKE) -C libfosfat uninstall
	$(MAKE) -C libfosgra uninstall

uninstall-pkgconfig:
	rm -f $(PKGCONFIG_DIR)/$(PKGCONFIG_FILE)

uninstall-fosmount:
	$(MAKE) -C fosmount uninstall

uninstall-tools:
	$(MAKE) -C tools uninstall

uninstall-docs:
	$(MAKE) -C DOCS uninstall

.PHONY: *clean *install* docs fosmount tools

dist:
	-$(RM) $(DISTFILE)
	dist=$(shell pwd)/fosfat-$(VERSION) && \
	for subdir in . $(SUBDIRS); do \
		mkdir -p "$$dist/$$subdir"; \
		$(MAKE) -C $$subdir dist-all DIST="$$dist/$$subdir"; \
	done && \
	tar cjf $(DISTFILE) fosfat-$(VERSION)
	-$(RM) -rf fosfat-$(VERSION)

dist-all:
	cp $(EXTRADIST) Makefile $(DIST)

.PHONY: dist dist-all
